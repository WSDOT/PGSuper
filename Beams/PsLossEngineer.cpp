///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2002  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#include <Reporting\ReportStyleHolder.h>
#include <PgsExt\PointOfInterest.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\AutoProgress.h>
#include <PgsExt\GirderLabel.h>

#include <Material\PsStrand.h>

#include "ElasticShorteningTable.h"
#include "FrictionLossTable.h"
#include "PostTensionInteractionTable.h"
#include "EffectOfPostTensionedTemporaryStrandsTable.h"
#include "TimeDependentLossesAtShippingTable.h"
#include "PostTensionTimeDependentLossesAtShippingTable.h"
#include "ElasticGainDueToDeckPlacementTable.h"
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
#include "RelaxationAfterTransferTable.h"
#include "FinalPrestressLossTable.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


template <class T>
void ReportRow(T* pTable,rptChapter* pChapter,IBroker* pBroker,int row,LOSSDETAILS& details,IDisplayUnits* pDispUnits,Uint16 level)
{
   if ( pTable == NULL )
      return;

   pTable->AddRow(pChapter,pBroker,row,details,pDispUnits,level);
}

/////////////////////////////////////////////////////////////////////////////
// CPsLossEngineer
LOSSDETAILS CPsLossEngineer::ComputeLosses(IBroker* pBroker,long agentID,BeamType beamType,const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE2(pBroker,IBridge,pBridge);
   const GDRCONFIG& config = pBridge->GetGirderConfiguration(span,gdr);

   return ComputeLosses(pBroker,agentID,beamType,poi,config);
}

LOSSDETAILS CPsLossEngineer::ComputeLosses(IBroker* pBroker,long agentID,BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   LOSSDETAILS details;

   GET_IFACE2(pBroker,IProgress,pProgress);
   pgsAutoProgress ap(pProgress);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GET_IFACE2(pBroker,IDisplayUnits,pDispUnits);
   const unitmgtLengthData& length = pDispUnits->GetSpanLengthUnit();

   std::ostringstream os;
   os << "Computing prestress losses Span "
      << LABEL_SPAN(span) << " Girder "
      << LABEL_GIRDER(gdr) << " at "
      << std::setw(length.Width)
      << std::setprecision(length.Precision) 
      << ::ConvertFromSysUnits(poi.GetDistFromStart(),length.UnitOfMeasure) << " " 
      << length.UnitOfMeasure.UnitTag() << " from start of girder" << std::ends;

   pProgress->UpdateMessage( os.str().c_str() );

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   switch (  pSpecEntry->GetLossMethod() )
   {
   case LOSSES_AASHTO_REFINED:
      LossesByRefinedEstimate(pBroker,agentID,beamType,poi,config,&details,laAASHTO);
      break;

   case LOSSES_WSDOT_REFINED:
      LossesByRefinedEstimate(pBroker,agentID,beamType,poi,config,&details,laWSDOT);
      break;

   case LOSSES_TXDOT_REFINED_2004:
      LossesByRefinedEstimate(pBroker,agentID,beamType,poi,config,&details,laTxDOT);
      break;

   case LOSSES_AASHTO_LUMPSUM:
      LossesByApproxLumpSum(pBroker,agentID,beamType,poi,config,&details,false);
      break;

   case LOSSES_WSDOT_LUMPSUM:
      LossesByApproxLumpSum(pBroker,agentID,beamType,poi,config,&details,true);
      break;

   case LOSSES_GENERAL_LUMPSUM:
      LossesByGeneralLumpSum(pBroker,agentID,beamType,poi,config,&details);
      break;

   default:
      CHECK(false); // Should never get here
   }

   return details;
}

LOSSDETAILS CPsLossEngineer::ComputeLossesForDesign(IBroker* pBroker,long agentID,BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   LOSSDETAILS details;

   m_bComputingLossesForDesign = true;
   details = ComputeLosses(pBroker,agentID,beamType,poi,config);
   m_bComputingLossesForDesign = false;

   return details;
}

void CPsLossEngineer::BuildReport(IBroker* pBroker,BeamType beamType,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnits)
{
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   Uint16 level = 0;
   switch( pSpecEntry->GetLossMethod() )
   {
   case LOSSES_AASHTO_REFINED:
      ReportRefinedMethod(pBroker,beamType,span,gdr,pChapter,pDispUnits,level,laAASHTO);
      break;

   case LOSSES_WSDOT_REFINED:
      ReportRefinedMethod(pBroker,beamType,span,gdr,pChapter,pDispUnits,level,laWSDOT);
      break;

   case LOSSES_TXDOT_REFINED_2004:
      ReportRefinedMethod(pBroker,beamType,span,gdr,pChapter,pDispUnits,level,laTxDOT);
      break;

   case LOSSES_AASHTO_LUMPSUM:
      ReportApproxLumpSumMethod(pBroker,beamType,span,gdr,pChapter,pDispUnits,level,false);
      break;

   case LOSSES_WSDOT_LUMPSUM:
      ReportApproxLumpSumMethod(pBroker,beamType,span,gdr,pChapter,pDispUnits,level,true);
      break;

   case LOSSES_GENERAL_LUMPSUM:
      ReportGeneralLumpSumMethod(pBroker,beamType,span,gdr,pChapter,pDispUnits,level);
      break;

   default:
      CHECK(false); // Should never get here
   }
}

void CPsLossEngineer::ReportRefinedMethod(IBroker* pBroker,BeamType beamType,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnits,Uint16 level,LossAgency lossAgency)
{
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ||
        lossAgency==laTxDOT)
   {
      ReportRefinedMethodBefore2005(pChapter,pBroker,beamType,span,gdr,pDispUnits,level);
   }
   else
   {
      ReportRefinedMethod2005(pChapter,pBroker,beamType,span,gdr,pDispUnits,level);
   }
}

void CPsLossEngineer::ReportApproxLumpSumMethod(IBroker* pBroker,BeamType beamType,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnits,Uint16 level,bool isWsdot)
{
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 )
   {
      ReportApproxMethod(pChapter,pBroker,beamType,span,gdr,pDispUnits,level,isWsdot);
   }
   else
   {
      ReportApproxMethod2005(pChapter,pBroker,beamType,span,gdr,pDispUnits,level);
   }
}

void CPsLossEngineer::ReportGeneralLumpSumMethod(IBroker* pBroker,BeamType beamType,SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnits,Uint16 level)
{
   ReportLumpSumMethod(pChapter,pBroker,beamType,span,gdr,pDispUnits,level);
}

void CPsLossEngineer::LossesByRefinedEstimate(IBroker* pBroker,long agentID,BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,LossAgency lossAgency)
{
   PRECONDITION(pLosses != 0 );

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ||
        lossAgency==laTxDOT)
   {
      LossesByRefinedEstimateBefore2005(pBroker,agentID,beamType,poi,config,pLosses);
   }
   else
   {
      LossesByRefinedEstimate2005(pBroker,agentID,beamType,poi,config,pLosses,lossAgency);
   }
}

void CPsLossEngineer::LossesByRefinedEstimateBefore2005(IBroker* pBroker,long agentID,BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses)
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
   Float64 Ad;
   Float64 ed;
   Float64 eperm;// eccentricity of the permanent strands on the non-composite section
   Float64 etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   Float64 Madlg;
   Float64 Msidl;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;
   StrandIndexType Ns, Nh, Nt;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length, end_size;

   Float64 Aslab;
   Float64 Pslab;
   lrfdLosses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(pBroker,poi,config,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &Ad, &ed,
                     &eperm, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // get time to prestress transfer
   GET_IFACE2(pBroker, ISpecification,   pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 shipping_loss = pSpecEntry->GetShippingLosses();

   lrfdRefinedLosses losses(poi.GetDistFromStart(),
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

                                Ag,
                                Ig,
                                Ybg,
                                Ac,
                                Ic,
                                Ybc,

                                rh,
                                ti,
                                shipping_loss
                                );


   // Any of the "get" methods on lrfdPsLosses can throw an lrfdXPsLosses exception if
   // the input data is bad.  To make sure we have everything correct, lets request
   // the elastic shortening losses and make sure an exception doesn't throw.
   try
   {
      Float64 pES = losses.PermanentStrand_ElasticShorteningLosses();
      pLosses->RefinedLosses = losses;
      pLosses->pLosses = &pLosses->RefinedLosses;
   }
   catch( const lrfdXPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::string msg;

      GET_IFACE2(pBroker,IStatusCenter,pStatusCenter);
      pgsStatusItem* pStatusItem;

      if ( e.GetReasonCode() == lrfdXPsLosses::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = "Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b of LRFD 3rd Edition 2004)\nAdjust the prestress jacking forces";
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,agentID,114,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = "Prestress losses could not be computed because the concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1";
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,2,agentID,114,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = "Prestress losses could not be computed because an unspecified error occured";
         pStatusItem = new pgsUnknownErrorStatusItem(agentID,112,__FILE__,__LINE__,msg.c_str());
      }

      pStatusCenter->Add(pStatusItem);

      msg += std::string("\nSee Status Center for Details");
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByRefinedEstimate2005(IBroker* pBroker,long agentID,BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,LossAgency lossAgency)
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
   Float64 Ad;
   Float64 ed;
   Float64 eperm;// eccentricity of the permanent strands on the non-composite section
   Float64 etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   Float64 Madlg;
   Float64 Msidl;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;
   StrandIndexType Ns, Nh, Nt;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length, end_size;

   Float64 Aslab;
   Float64 Pslab;
   lrfdLosses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(pBroker,poi,config,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &Ad, &ed,
                     &eperm, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // get time to prestress transfer
   GET_IFACE2(pBroker, ISpecification,   pSpec);
   GET_IFACE2(pBroker, ILibrary,         pLib);
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   if ( lossAgency==laWSDOT )
   {
      pLosses->Method = LOSSES_WSDOT_REFINED_2005;
   }
   else
   {
      pLosses->Method = LOSSES_AASHTO_REFINED_2005;
   }

   lrfdRefinedLosses2005 losses(poi.GetDistFromStart(),
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
                                Ag, // volume/length
                                perimeter, // surface area/length
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
                                Mdlg,
                                Madlg,
                                Msidl,
                                rh,
                                ti,
                                th,
                                td,
                                tf,
                                pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal,
                                pSpecEntry->GetCuringMethodTimeAdjustmentFactor(),
                                lossAgency!=laWSDOT);


   // Any of the "get" methods on lrfdPsLosses can throw an lrfdXPsLosses exception if
   // the input data is bad.  To make sure we have everything correct, lets request
   // the elastic shortening losses and make sure an exception doesn't throw.
   try
   {
      Float64 pES = losses.PermanentStrand_ElasticShorteningLosses();
      pLosses->RefinedLosses2005 = losses;
      pLosses->pLosses = &pLosses->RefinedLosses2005;
   }
   catch( const lrfdXPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::string msg;

      GET_IFACE2(pBroker,IStatusCenter,pStatusCenter);
      pgsStatusItem* pStatusItem;

      if ( e.GetReasonCode() == lrfdXPsLosses::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = "Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b of LRFD 3rd Edition 2004)\nAdjust the prestress jacking forces";
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,agentID,114,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = "Prestress losses could not be computed because the concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1";
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,2,agentID,114,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = "Prestress losses could not be computed because an unspecified error occured";
         pStatusItem = new pgsUnknownErrorStatusItem(agentID,112,__FILE__,__LINE__,msg.c_str());
      }

      pStatusCenter->Add(pStatusItem);

      msg += std::string("\nSee Status Center for Details");
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByApproxLumpSum(IBroker* pBroker,long agentID,BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses,bool isWsdot)
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
   Float64 Ad;
   Float64 ed;
   Float64 eperm;// eccentricity of the permanent strands on the non-composite section
   Float64 etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   Float64 Madlg;
   Float64 Msidl;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;
   StrandIndexType Ns, Nh, Nt;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length, end_size;

   Float64 Aslab;
   Float64 Pslab;
   lrfdLosses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(pBroker,poi,config,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &Ad, &ed,
                     &eperm, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // get time to prestress transfer
   GET_IFACE2(pBroker, ISpecification,   pSpec);
   GET_IFACE2(pBroker, ILibrary,         pLib);
   std::string spec_name = pSpec->GetSpecification();
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
            GET_IFACE2(pBroker,ILongRebarGeometry,pLongRebarGeom);
            ppr = pLongRebarGeom->GetPPRBottomHalf(poi);
            pLosses->Method = LOSSES_AASHTO_LUMPSUM;
         }

         Float64 shipping_loss = pSpecEntry->GetShippingLosses();

         lrfdApproximateLosses losses(
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

                            fc,
                            fci,
                            fcSlab,
                            Ec,   // Modulus of elasticity of girder
                            Eci,  // Modulus of elasticity of girder at transfer
                            EcSlab,  // Modulus of elasticity of deck

                            Mdlg,  // Dead load moment of girder only
                            Madlg,  // Additional dead load on girder section
                            Msidl, // Superimposed dead loads
                         
                            Ag,
                            Ig,
                            Ybg,
                            Ac,
                            Ic,
                            Ybc,

                            rh,      // relative humidity
                            ti,   // Time until prestress transfer
                            !isWsdot);

         // Any of the "get" methods on lrfdPsLosses can throw an lrfdXPsLosses exception if
         // the input data is bad.  To make sure we have everything correct, lets request
         // the elastic shortening losses and make sure an exception doesn't throw.
         Float64 pES = losses.PermanentStrand_ElasticShorteningLosses();
         pLosses->ApproxLosses  = losses;
         pLosses->pLosses = &pLosses->ApproxLosses;
      }
      else
      {
         // 3rd edition /w 2005 interims and later
         lrfdApproximateLosses2005 losses(poi.GetDistFromStart(), // location along girder where losses are computed
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
                         
                            Ag,
                            Ig,
                            Ybg,
                            Ac,
                            Ic,
                            Ybc,

                            rh,      // relative humidity
                            ti,   // Time until prestress transfer
                            !isWsdot
                            );

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
         Float64 pES = losses.PermanentStrand_ElasticShorteningLosses();
         pLosses->ApproxLosses2005  = losses;
         pLosses->pLosses = &pLosses->ApproxLosses2005;
      }

   } // end of try block
   catch( const lrfdXPsLosses& e )
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::string msg;

      GET_IFACE2(pBroker,IStatusCenter,pStatusCenter);
      pgsStatusItem* pStatusItem;

      if ( e.GetReasonCode() == lrfdXPsLosses::fpjOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = "Prestress losses could not be computed because the prestress jacking stress fpj does not exceed 0.5fpu (see Article 5.9.5.4.4b of LRFD 3rd Edition 2004)\nAdjust the prestress jacking forces";
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,agentID,114,msg.c_str());
      }
      else if ( e.GetReasonCode() == lrfdXPsLosses::fcOutOfRange )
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = "Prestress losses could not be computed because the concrete strength is out of range per LRFD 5.4.2.1 and 5.9.5.1";
         pStatusItem = new pgsGirderDescriptionStatusItem(span,gdr,1,agentID,114,msg.c_str());
      }
      else
      {
         reason |= XREASON_ASSUMPTIONVIOLATED;
         msg = "Prestress losses could not be computed because an unspecified error occured";
         pStatusItem = new pgsUnknownErrorStatusItem(agentID,112,__FILE__,__LINE__,msg.c_str());
      }

      pStatusCenter->Add(pStatusItem);

      msg += std::string("\nSee Status Center for Details");
      THROW_UNWIND(msg.c_str(),reason);
   }
}

void CPsLossEngineer::LossesByGeneralLumpSum(IBroker* pBroker,long agentID,BeamType beamType,const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses)
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
   Float64 Ad;
   Float64 ed;
   Float64 eperm;// eccentricity of the permanent strands on the non-composite section
   Float64 etemp;
   Float64 aps;  // area of one prestress strand
   Float64 ApsPerm;
   Float64 ApsTTS;
   Float64 Mdlg;
   Float64 Madlg;
   Float64 Msidl;
   Float64 rh;
   Float64 ti,th,td,tf; // initial time, time of hauling,time of deck placment, final time

   Float64 PjS, PjH, PjT;
   StrandIndexType Ns, Nh, Nt;

   Float64 fci,fc,fcSlab;
   Float64 Eci,Ec,EcSlab;

   Float64 girder_length, span_length, end_size;

   Float64 Aslab;
   Float64 Pslab;
   lrfdLosses::TempStrandUsage usage;

   Float64 anchorSet,wobble,coeffFriction,angleChange;

   GetLossParameters(pBroker,poi,config,
                     &grade, &type, &fpjPerm, &fpjTTS, &perimeter, &Ag, &Ig, &Ybg, &Ac, &Ic, &Ybc, &Ad, &ed,
                     &eperm, &etemp, &aps, &ApsPerm, &ApsTTS, &Mdlg, &Madlg, &Msidl, &rh, 
                     &ti, &th, &td,& tf, &PjS, &PjH, &PjT,
                     &Ns, &Nh, &Nt,
                     &fci,&fc,&fcSlab,
                     &Eci,&Ec,&EcSlab,
                     &girder_length,&span_length,&end_size,
                     &Aslab,&Pslab,&usage,&anchorSet,&wobble,&coeffFriction,&angleChange);

   pLosses->Method = LOSSES_GENERAL_LUMPSUM;

   // If there are no strands then there can't be any losses
   StrandIndexType Nstrands = Ns + Nh + Nt;

   if ( Nstrands == 0 )
   {
      pLosses->LumpSum = lrfdLumpSumLosses(0,0,0,0,usage,0,0,0,0,0,0,0,0);
      pLosses->pLosses = &(pLosses->LumpSum);
   }
   else
   {
      GET_IFACE2(pBroker,ISpecification,pSpec);
      std::string strSpecName = pSpec->GetSpecification();

      GET_IFACE2(pBroker,ILibrary,pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

      pLosses->LumpSum = lrfdLumpSumLosses(ApsPerm,ApsTTS,fpjPerm,fpjTTS,usage,
                                           pSpecEntry->GetBeforeXferLosses(),
                                           pSpecEntry->GetAfterXferLosses(),
                                           pSpecEntry->GetLiftingLosses(),
                                           pSpecEntry->GetShippingLosses(),
                                           pSpecEntry->GetBeforeTempStrandRemovalLosses(),
                                           pSpecEntry->GetAfterTempStrandRemovalLosses(),
                                           pSpecEntry->GetAfterDeckPlacementLosses(),
                                           pSpecEntry->GetFinalLosses());

      pLosses->pLosses = &(pLosses->LumpSum);
   }
}


void CPsLossEngineer::ReportRefinedMethodBefore2005(rptChapter* pChapter,IBroker* pBroker,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level)
{
   rptParagraph* pParagraph;

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Refined Estimate of Time-Dependent Losses [5.9.5.4]" << rptNewLine;

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite3, span, gdr, POI_TABULAR );
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( pgsTypes::CastingYard, span, gdr, POI_TABULAR | POI_PICKPOINT, POIFIND_OR);
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

   GET_IFACE2(pBroker,ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   // Do some preliminary setup for the tables.
   INIT_UV_PROTOTYPE( rptForceUnitValue,   force,       pDispUnits->GetGeneralForceUnit(),    false );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDispUnits->GetAreaUnit(),            false );
   INIT_UV_PROTOTYPE( rptLength4UnitValue, mom_inertia, pDispUnits->GetMomentOfInertiaUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  ecc,         pDispUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cg,          pDispUnits->GetComponentDimUnit(),    false );
   INIT_UV_PROTOTYPE( rptMomentUnitValue,  moment,      pDispUnits->GetMomentUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDispUnits->GetStressUnit(),          false );
   INIT_UV_PROTOTYPE( rptStressUnitValue,  mod_e,       pDispUnits->GetModEUnit(),            false );

   bool bTemporaryStrands = ( 0 < Nt && girderData.TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   // Relaxation At Prestress Transfer
   ReportInitialRelaxation(pChapter,pBroker,bTemporaryStrands,details.pLosses,pDispUnits,level);

   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,pDispUnits,level);
   CFrictionLossTable*                          pFR  = NULL;
   CPostTensionInteractionTable*                pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = NULL;
   
   if ( 0 < Nt && girderData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,pBroker,span,gdr,details,pDispUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = NULL;
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = NULL;

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if ( pGirderHaulingSpecCriteria->IsHaulingCheckEnabled() )
   {
      pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,pDispUnits,level);

      if ( 0 < Nt ) 
         pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   }

   CTemporaryStrandRemovalTable*        pPTR = NULL;
   if (0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Time dependent losses" << rptNewLine;

   CChangeOfConcreteStressTable*   pDeltaFcdp = CChangeOfConcreteStressTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   CCreepAndShrinkageTable*        pCR        = CCreepAndShrinkageTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   CRelaxationAfterTransferTable*  pR2        = CRelaxationAfterTransferTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   CFinalPrestressLossTable*       pT         = CFinalPrestressLossTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

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
      ReportLocation(pES,  row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pFR,  row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTT, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTP, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPSH, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTH, row1,poi,stage,end_size,pDispUnits);

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportLocation(pPTR,      row2,poi,end_size,pDispUnits);
         ReportLocation(pDeltaFcdp,row2,poi,end_size,pDispUnits);
         ReportLocation(pCR,       row2,poi,end_size,pDispUnits);
         ReportLocation(pR2,       row2,poi,end_size,pDispUnits);
         ReportLocation(pT,        row2,poi,end_size,pDispUnits);
      }

      // fill each row1 with data
      if ( !bSkipToNextRow )
      {
         details = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pFR, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTT,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTP,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPSH,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTH,pChapter,pBroker,row1,details,pDispUnits,level);
      }

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportRow(pPTR,      pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pDeltaFcdp,pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pCR,       pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pR2,       pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pT,        pChapter,pBroker,row2,details,pDispUnits,level);

         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportRefinedMethod2005(rptChapter* pChapter,IBroker* pBroker,BeamType beamType,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level)
{
   rptParagraph* pParagraph;

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Refined Estimate of Time-Dependent Losses [5.9.5.4]" << rptNewLine;

#if defined IGNORE_2007_CHANGES
   if ( lrfdVersionMgr::FourthEdition2007 <= pSpecEntry->GetSpecificationType() )
   {
      pParagraph = new rptParagraph();
      *pChapter << pParagraph;
      *pParagraph << color(Red) << bold(ON) << "Changes to LRFD 4th Edition, 2007, Article 5.4.2.3.2 have been ignored." << bold(OFF) << color(Black) << rptNewLine;
   }
#endif

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite3, span, gdr, POI_TABULAR );
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( pgsTypes::CastingYard, span, gdr, POI_TABULAR | POI_PICKPOINT, POIFIND_OR );
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

   GET_IFACE2(pBroker,ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);


   bool bTemporaryStrands = ( 0 < Nt && girderData.TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   ReportInitialRelaxation(pChapter,pBroker,bTemporaryStrands,details.pLosses,pDispUnits,level);

   ////////////////////////////////////////////////////////////////////////////////////////
   // Create the tables for losses - order is important here... The factory methods
   // put content into the chapter and return a table that needs to be filled up
   CElasticShorteningTable*                     pES  = CElasticShorteningTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,pDispUnits,level);
   CFrictionLossTable*                          pFR  = NULL;
   CPostTensionInteractionTable*                pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable* pPTP = NULL;

   

   if ( 0 < Nt && girderData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,pBroker,span,gdr,details,pDispUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   }

   CShrinkageAtHaulingTable*                       pSRH = NULL;
   CCreepAtHaulingTable*                           pCRH = NULL;
   CRelaxationAtHaulingTable*                      pR1H = NULL;
   CTimeDependentLossesAtShippingTable*            pPSH = NULL;
   
   CPostTensionTimeDependentLossesAtShippingTable* pPTH  = NULL;

   if ( pGirderHaulingSpecCriteria->IsHaulingCheckEnabled() )
   {
      pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pParagraph;
      *pParagraph << "Time dependent losses between transfer and hauling [5.9.5.4.2]" << rptNewLine << rptNewLine;

      pSRH = CShrinkageAtHaulingTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,details,pDispUnits,level);
      pCRH = CCreepAtHaulingTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,details,pDispUnits,level);
      pR1H = CRelaxationAtHaulingTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,details,pDispUnits,level);
      pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,pDispUnits,level);

      if ( 0 < Nt )
      {
         pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
      }
   }


   ///////////////////////////////////////////////////////////////////////////////////////////
   // Time-dependent losses between transfer and deck placement
   ///////////////////////////////////////////////////////////////////////////////////////////

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
      *pParagraph << "Time dependent losses between transfer and deck placement [5.9.5.4.2]" << rptNewLine;
   else
      *pParagraph << "Time dependent losses between transfer and installation of precast members [5.9.5.4.2, 5.9.5.4.4]" << rptNewLine;

   CShrinkageAtDeckPlacementTable*      pSR = CShrinkageAtDeckPlacementTable::PrepareTable(pChapter,pBroker,span,gdr,details,pDispUnits,level);
   CCreepAtDeckPlacementTable*          pCR = CCreepAtDeckPlacementTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   CRelaxationAtDeckPlacementTable*     pR1 = CRelaxationAtDeckPlacementTable::PrepareTable(pChapter,pBroker,span,gdr,details,pDispUnits,level);
   CTimeDependentLossesAtDeckPlacementTable* pLTid = CTimeDependentLossesAtDeckPlacementTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   CTemporaryStrandRemovalTable*        pPTR = NULL;
   
   if ( 0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   ///////////////////////////////////////////////////////////////////////////////////////////
   // Time-dependent losses between deck placement and final time
   ///////////////////////////////////////////////////////////////////////////////////////////

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if ( pBridge->IsCompositeDeck() )
      *pParagraph << "Losses: Time of Deck Placement to Final Time [5.9.5.4.3]" << rptNewLine;
   else
      *pParagraph << "Losses: Time of Installation of Precast Members to Final Time [5.9.5.4.3, 5.9.5.4.4]" << rptNewLine;

   CElasticGainDueToDeckPlacementTable*            pED  = CElasticGainDueToDeckPlacementTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   
   CShrinkageAtFinalTable*      pSD = CShrinkageAtFinalTable::PrepareTable(pChapter,pBroker,span,gdr,details,pDispUnits,level);
   CCreepAtFinalTable*          pCD = CCreepAtFinalTable::PrepareTable(pChapter,pBroker,span,gdr,details,pDispUnits,level);
   CRelaxationAtFinalTable*     pR2 = CRelaxationAtFinalTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   CDeckShrinkageLossTable*      pSS   = CDeckShrinkageLossTable::PrepareTable(pChapter,pBroker,span,gdr,details,pDispUnits,level);
   CTimeDependentLossFinalTable* pLTdf = CTimeDependentLossFinalTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   CTimeDependentLossesTable*    pLT   = CTimeDependentLossesTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   CTotalPrestressLossTable*     pT    = CTotalPrestressLossTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

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
      ReportLocation(pES,  row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pFR,  row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTT, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTP, row1,poi,stage,end_size,pDispUnits);

      ReportLocation(pSRH, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pCRH, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pR1H, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPSH, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTH, row1,poi,stage,end_size,pDispUnits);

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportLocation(pSR,  row2,poi,end_size,pDispUnits);
         ReportLocation(pCR,  row2,poi,end_size,pDispUnits);
         ReportLocation(pR1,  row2,poi,end_size,pDispUnits);
         ReportLocation(pLTid,row2,poi,end_size,pDispUnits);
         ReportLocation(pPTR, row2,poi,end_size,pDispUnits);
         ReportLocation(pED,  row2,poi,end_size,pDispUnits);
         ReportLocation(pSD,  row2,poi,end_size,pDispUnits);
         ReportLocation(pCD,  row2,poi,end_size,pDispUnits);
         ReportLocation(pR2,  row2,poi,end_size,pDispUnits);
         ReportLocation(pSS,  row2,poi,end_size,pDispUnits);
         ReportLocation(pLTdf,row2,poi,end_size,pDispUnits);
         ReportLocation(pLT,  row2,poi,end_size,pDispUnits);
         ReportLocation(pT,   row2,poi,end_size,pDispUnits);
      }

      // fill each row1 with data
      if ( !bSkipToNextRow )
      {
         details = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pFR, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTT,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTP,pChapter,pBroker,row1,details,pDispUnits,level);

         ReportRow(pSRH, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pCRH, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pR1H, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPSH, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTH, pChapter,pBroker,row1,details,pDispUnits,level);
      }

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportRow(pSR,  pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pCR,  pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pR1,  pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pLTid,pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pPTR, pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pED,  pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pSD,  pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pCD,  pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pR2,  pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pSS,  pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pLTdf,pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pLT,  pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pT,   pChapter,pBroker,row2,details,pDispUnits,level);

         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportApproxMethod(rptChapter* pChapter,IBroker* pBroker,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level,bool isWsdot)
{
   rptParagraph* pParagraph;

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite3, span, gdr, POI_TABULAR );
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( pgsTypes::CastingYard, span, gdr, POI_TABULAR | POI_PICKPOINT, POIFIND_OR );
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

   GET_IFACE2(pBroker,ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   if (isWsdot)
      *pParagraph << "Approximate Lump Sum Estimate of Time-Dependent Losses [WSDOT BDM 6.1.5.B3]" << rptNewLine;
   else
      *pParagraph << "Approximate Lump Sum Estimate of Time-Dependent Losses [5.9.5.3]" << rptNewLine;

   bool bTemporaryStrands = ( 0 < Nt && girderData.TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   //////////////////////////////////////////////////////////
   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,pBroker,bTemporaryStrands,details.pLosses,pDispUnits,level);

   CElasticShorteningTable*                        pES  = CElasticShorteningTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,pDispUnits,level);

   ReportLumpSumTimeDependentLossesAtShipping(pChapter,pBroker,details,pDispUnits,level);

   CFrictionLossTable*                             pFR  = NULL;
   CPostTensionInteractionTable*                   pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable*    pPTP = NULL;

   if ( 0 < Nt && girderData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,pBroker,span,gdr,details,pDispUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,pDispUnits,level);
   CPostTensionTimeDependentLossesAtShippingTable* pPTH  = NULL;
   
   if ( 0 < Nt )
      pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   CTemporaryStrandRemovalTable*                   pPTR = NULL;
   if ( 0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   ReportLumpSumTimeDependentLosses(pChapter,pBroker,details,pDispUnits,level);

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   
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

      ReportLocation(pES,  row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pFR,  row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTT, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTP, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPSH, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTH, row1,poi,stage,end_size,pDispUnits);

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportLocation(pPTR,row2,poi,end_size,pDispUnits);
         ReportLocation(pT,  row2,poi,end_size,pDispUnits);
      }

      if ( !bSkipToNextRow )
      {
         details = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pFR, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTT,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTP,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPSH,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTH,pChapter,pBroker,row1,details,pDispUnits,level);
      }

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportRow(pPTR,pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pT,  pChapter,pBroker,row2,details,pDispUnits,level);
         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}

void CPsLossEngineer::ReportApproxMethod2005(rptChapter* pChapter,IBroker* pBroker,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level)
{
   rptParagraph* pParagraph;

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span,gdr );

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> bsPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite3, span, gdr, POI_TABULAR );
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( pgsTypes::CastingYard, span, gdr, POI_TABULAR | POI_PICKPOINT, POIFIND_OR );
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

   GET_IFACE2(pBroker,ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Approximate Estimate of Time-Dependent Losses [5.9.5.3]" << rptNewLine;

   bool bTemporaryStrands = ( 0 < Nt && girderData.TempStrandUsage == pgsTypes::ttsPretensioned ? true : false);

   //////////////////////////////////////////////////////////
   // NOTE: The order of everything from here to the loop is important. 
   // It is the order in which things are put into the report
   ReportInitialRelaxation(pChapter,pBroker,bTemporaryStrands,details.pLosses,pDispUnits,level);

   CElasticShorteningTable*                        pES  = CElasticShorteningTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,pDispUnits,level);

   ReportLumpSumTimeDependentLossesAtShipping(pChapter,pBroker,details,pDispUnits,level);

   CFrictionLossTable*                             pFR  = NULL;
   CPostTensionInteractionTable*                   pPTT = NULL;
   CEffectOfPostTensionedTemporaryStrandsTable*    pPTP = NULL;

   if ( 0 < Nt && girderData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      pFR  = CFrictionLossTable::PrepareTable(pChapter,pBroker,span,gdr,details,pDispUnits,level);
      pPTT = CPostTensionInteractionTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
      pPTP = CEffectOfPostTensionedTemporaryStrandsTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);
   }

   CTimeDependentLossesAtShippingTable*            pPSH = CTimeDependentLossesAtShippingTable::PrepareTable(pChapter,pBroker,span,gdr,bTemporaryStrands,pDispUnits,level);
   CPostTensionTimeDependentLossesAtShippingTable* pPTH = NULL;

   if ( 0 < Nt )
      pPTH = CPostTensionTimeDependentLossesAtShippingTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   CElasticGainDueToDeckPlacementTable*            pED  = CElasticGainDueToDeckPlacementTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   CTemporaryStrandRemovalTable*                   pPTR = NULL;
   if ( 0 < Nt )
      pPTR = CTemporaryStrandRemovalTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   ReportLumpSumTimeDependentLosses(pChapter,pBroker,details,pDispUnits,level);

   CTotalPrestressLossTable* pT = CTotalPrestressLossTable::PrepareTable(pChapter,pBroker,span,gdr,pDispUnits,level);

   PoiSet::iterator siter;

   pgsPointOfInterest prev_poi(0,0,0);
   bool bSkipToNextRow = false;
   int row1 = 1;
   int row2 = 1;
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

      ReportLocation(pES,  row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pFR,  row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTT, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTP, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPSH, row1,poi,stage,end_size,pDispUnits);
      ReportLocation(pPTH, row1,poi,stage,end_size,pDispUnits);

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportLocation(pED, row2,poi,end_size,pDispUnits);
         ReportLocation(pPTR,row2,poi,end_size,pDispUnits);
         ReportLocation(pT,  row2,poi,end_size,pDispUnits);
      }

      if ( !bSkipToNextRow )
      {
         details = pILosses->GetLossDetails( poi );

         ReportRow(pES, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pFR, pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTT,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTP,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPSH,pChapter,pBroker,row1,details,pDispUnits,level);
         ReportRow(pPTH,pChapter,pBroker,row1,details,pDispUnits,level);
      }

      if ( stage == pgsTypes::BridgeSite3 )
      {
         ReportRow(pED, pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pPTR,pChapter,pBroker,row2,details,pDispUnits,level);
         ReportRow(pT,  pChapter,pBroker,row2,details,pDispUnits,level);
         row2++;
      }
      
      row1++;
      prev_poi = poi;
   }
}


void CPsLossEngineer::ReportLumpSumMethod(rptChapter* pChapter,IBroker* pBroker,CPsLossEngineer::BeamType beamType,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnits,Uint16 level)
{
   rptParagraph* pParagraph;

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary);

   pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "General Lump Sum Estimate Losses" << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(2,"");
   table->SetColumnWidth(0,3.0);
   table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
   table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   *pParagraph << table << rptNewLine;

   // Do some preliminary setup for the tables.
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDispUnits->GetStressUnit(), false );

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> cyPoi = pIPoi->GetPointsOfInterest( pgsTypes::CastingYard, span, gdr, POI_TABULAR );

   GET_IFACE2(pBroker,ILosses,pILosses);
   LOSSDETAILS details = pILosses->GetLossDetails( cyPoi[0] );

   int row = 0;

   (*table)(row,0) << "Stage"; (*table)(row++,1) << COLHDR("Loss", rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*table)(row,0) << "Before prestress transfer"; (*table)(row++,1) << stress.SetValue(details.LumpSum.GetBeforeXferLosses());
   (*table)(row,0) << "After prestress transfer";  (*table)(row++,1) << stress.SetValue(details.LumpSum.GetAfterXferLosses());
   (*table)(row,0) << "At girder lifting";         (*table)(row++,1) << stress.SetValue(details.LumpSum.GetLiftingLosses());
   (*table)(row,0) << "At girder shipping";        (*table)(row++,1) << stress.SetValue(details.LumpSum.GetShippingLosses());

   if ( 0 < NtMax )
   {
      (*table)(row,0) << "Before temporary strand removal"; (*table)(row++,1) << stress.SetValue(details.LumpSum.GetBeforeTempStrandRemovalLosses());
      (*table)(row,0) << "After temporary strand removal";  (*table)(row++,1) << stress.SetValue(details.LumpSum.GetAfterTempStrandRemovalLosses());
   }

   (*table)(row,0) << "After deck placement";            (*table)(row++,1) << stress.SetValue(details.LumpSum.GetAfterDeckPlacementLosses());

   (*table)(row,0) << "Final";  (*table)(row++,1) << stress.SetValue(details.LumpSum.GetFinalLosses());
}


//////////////////////////////////////////////////
// Utility functions for reporting
void CPsLossEngineer::ReportInitialRelaxation(rptChapter* pChapter,IBroker* pBroker,bool bTemporaryStrands,const lrfdLosses* pLosses,IDisplayUnits* pDispUnits,Uint16 level)
{
   if ( pLosses->IgnoreInitialRelaxation() )
      return; // nothing to do

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDispUnits->GetStressUnit(),          false );

   // Relaxation At Prestress Transfer
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Prestress loss due to relaxation before transfer" << rptNewLine;

   if ( pLosses->GetStrandType() == matPsStrand::LowRelaxation )
      *pParagraph << rptRcImage(strImagePath + "Delta_FpR0_LR.gif") << rptNewLine;
   else 
      *pParagraph << rptRcImage(strImagePath + "Delta_FpR0_SR.gif") <<rptNewLine;

   rptRcTable* table;

   if ( bTemporaryStrands )
   {
      table = pgsReportStyleHolder::CreateDefaultTable(4,"Temporary Strands");
      *pParagraph << table << rptNewLine;

      (*table)(0,0) << "t" << rptNewLine << "(Days)";
      (*table)(0,1) << COLHDR(RPT_FPJ, rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*table)(0,2) << COLHDR(RPT_FPY, rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*table)(0,3) << COLHDR(symbol(DELTA) << Sub2("f","pR0"), rptStressUnitTag, pDispUnits->GetStressUnit() );

      (*table)(1,0) << ::ConvertFromSysUnits( pLosses->GetInitialAge(), unitMeasure::Day );
      (*table)(1,1) << stress.SetValue( pLosses->GetFpjTemporary() );
      (*table)(1,2) << stress.SetValue( pLosses->GetFpy() );
      (*table)(1,3) << stress.SetValue( pLosses->TemporaryStrand_RelaxationLossesBeforeTransfer() );
   }

   table = pgsReportStyleHolder::CreateDefaultTable(4,"Permanent Strands");
   *pParagraph << table << rptNewLine;

   (*table)(0,0) << "t" << rptNewLine << "(Days)";
   (*table)(0,1) << COLHDR(RPT_FPJ, rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*table)(0,2) << COLHDR(RPT_FPY, rptStressUnitTag, pDispUnits->GetStressUnit() );
   (*table)(0,3) << COLHDR(symbol(DELTA) << Sub2("f","pR0"), rptStressUnitTag, pDispUnits->GetStressUnit() );

   (*table)(1,0) << ::ConvertFromSysUnits( pLosses->GetInitialAge(), unitMeasure::Day );
   (*table)(1,1) << stress.SetValue( pLosses->GetFpjPermanent() );
   (*table)(1,2) << stress.SetValue( pLosses->GetFpy() );
   (*table)(1,3) << stress.SetValue( pLosses->PermanentStrand_RelaxationLossesBeforeTransfer() );
}

void CPsLossEngineer::ReportLocation(rptRcTable* pTable,int row,const pgsPointOfInterest& poi,pgsTypes::Stage stage,Float64 endsize,IDisplayUnits* pDispUnits)
{
   if ( pTable == NULL )
      return;

   INIT_UV_PROTOTYPE( rptPointOfInterest,  spanloc,     pDispUnits->GetSpanLengthUnit(),      false );
   INIT_UV_PROTOTYPE( rptPointOfInterest,  gdrloc,      pDispUnits->GetSpanLengthUnit(),      false );

   gdrloc.MakeGirderPoi();

   int rowOffset = pTable->GetNumberOfHeaderRows() - 1;

   if ( stage == pgsTypes::CastingYard )
   {
      (*pTable)(row+rowOffset,0) << gdrloc.SetValue( poi );
      (*pTable)(row+rowOffset,1) << "";
   }
   else
   {
      (*pTable)(row+rowOffset,0) << "";
      (*pTable)(row+rowOffset,1) << spanloc.SetValue( poi, endsize );
   }
}

void CPsLossEngineer::ReportLocation(rptRcTable* pTable,int row,const pgsPointOfInterest& poi,Float64 endsize,IDisplayUnits* pDispUnits)
{
   if ( pTable == NULL )
      return;

   INIT_UV_PROTOTYPE( rptPointOfInterest,  spanloc,     pDispUnits->GetSpanLengthUnit(),      false );

   int rowOffset = pTable->GetNumberOfHeaderRows() - 1;
   (*pTable)(row+rowOffset,0) << spanloc.SetValue( poi, endsize );
}

void CPsLossEngineer::ReportLumpSumTimeDependentLossesAtShipping(rptChapter* pChapter,IBroker* pBroker,const LOSSDETAILS& details,IDisplayUnits* pDispUnits,Uint16 level)
{
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   // Lump Sum Loss at time of shipping
   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Approximate Lump Sum Estimate of Time Dependent Losses at Shipping" << rptNewLine;

   if ( details.Method == LOSSES_AASHTO_LUMPSUM || details.Method == LOSSES_WSDOT_LUMPSUM )
   {
      // Approximate methods before 2005
      GET_IFACE2(pBroker, ISpecification,   pSpec);
      GET_IFACE2(pBroker, ILibrary,         pLib);
      std::string spec_name = pSpec->GetSpecification();
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

      Float64 shipping_losses = pSpecEntry->GetShippingLosses();

      pParagraph = new rptParagraph;
      *pChapter << pParagraph;

      if ( shipping_losses < 0 )
      {
         // % of long term
         *pParagraph << symbol(DELTA) << Sub2("f","pLTH") << " = " << -1*shipping_losses << "(" << symbol(DELTA) << Sub2("f","pLT") << ")" << rptNewLine;
      }

      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDispUnits->GetStressUnit(),          true );
      *pParagraph << symbol(DELTA) << Sub2("f","pLTH") << " = " << stress.SetValue(details.pLosses->PermanentStrand_TimeDependentLossesAtShipping()) << rptNewLine;
   }
   else
   {
      // Approximate methods, 2005
      if ( pDispUnits->GetUnitDisplayMode() == pgsTypes::umSI )
         *pParagraph<< rptRcImage(strImagePath + "LumpSumLossEquation2005_SI_Shipping.gif") << rptNewLine;
      else
         *pParagraph<< rptRcImage(strImagePath + "LumpSumLossEquation2005_US_Shipping.gif") << rptNewLine;

      rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(9,"");

      INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDispUnits->GetAreaUnit(),            false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDispUnits->GetStressUnit(),          false );

      rptRcScalar scalar;
      scalar.SetFormat( sysNumericFormatTool::Automatic );
      scalar.SetWidth(6);
      scalar.SetPrecision(2);

      *pParagraph << table << rptNewLine;
      (*table)(0,0) << COLHDR(RPT_FCI, rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*table)(0,1) << Sub2(symbol(gamma),"st");
      (*table)(0,2) << "Relative" << rptNewLine << "Humidity (%)";
      (*table)(0,3) << Sub2(symbol(gamma),"h");
      (*table)(0,4) << COLHDR(Sub2("f","pi"), rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*table)(0,5) << COLHDR(Sub2("A","ps"), rptAreaUnitTag, pDispUnits->GetAreaUnit() );
      (*table)(0,6) << COLHDR(Sub2("A","g"), rptAreaUnitTag, pDispUnits->GetAreaUnit() );
      (*table)(0,7) << COLHDR(symbol(DELTA) << Sub2("f","pR"), rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*table)(0,8) << COLHDR(symbol(DELTA) << Sub2("f","pLTH"), rptStressUnitTag, pDispUnits->GetStressUnit() );

      (*table)(1,0) << stress.SetValue( details.pLosses->GetFci() );
      (*table)(1,1) << scalar.SetValue( details.ApproxLosses2005.GetStrengthFactor() );
      (*table)(1,2) << details.pLosses->GetRelHumidity();
      (*table)(1,3) << scalar.SetValue( details.ApproxLosses2005.GetHumidityFactor() );
      (*table)(1,4) << stress.SetValue( details.ApproxLosses2005.GetFpi() );
      (*table)(1,5) << area.SetValue( details.pLosses->GetApsPermanent() );
      (*table)(1,6) << area.SetValue( details.pLosses->GetAg() );
      (*table)(1,7) << stress.SetValue( details.ApproxLosses2005.PermanentStrand_RelaxationLossesAtXfer() );
      (*table)(1,8) << stress.SetValue( details.pLosses->PermanentStrand_TimeDependentLossesAtShipping() );
   }
}

void CPsLossEngineer::ReportLumpSumTimeDependentLosses(rptChapter* pChapter,IBroker* pBroker,const LOSSDETAILS& details,IDisplayUnits* pDispUnits,Uint16 level)
{
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Approximate Lump Sum Estimate of Time Dependent Losses" << rptNewLine;

   if ( details.Method == LOSSES_AASHTO_LUMPSUM || details.Method == LOSSES_WSDOT_LUMPSUM )
   {
      std::string strLossEqnImage[2][5][2][2]; 
      // dim 0... 0 = LRFD, 1 = WSDOT
      // dim 1... 0 = I Beam, 1 = U Beam, 2 = SolidSlab, 3 = Box Beams, 4 = Single T
      // dim 2... 0 = Low Relax, 1 = Stress Rel
      // dim 2... 0 = SI, 1 = US
      strLossEqnImage[0][lrfdApproximateLosses::IBeam][0][0]     = "ApproxLoss_LRFD_IBeam_LowRelax_SI.gif";
      strLossEqnImage[0][lrfdApproximateLosses::IBeam][1][0]     = "ApproxLoss_LRFD_IBeam_StressRel_SI.gif";
      strLossEqnImage[0][lrfdApproximateLosses::UBeam][0][0]     = "ApproxLoss_LRFD_UBeam_LowRelax_SI.gif";
      strLossEqnImage[0][lrfdApproximateLosses::UBeam][1][0]     = "ApproxLoss_LRFD_UBeam_StressRel_SI.gif";
      strLossEqnImage[0][lrfdApproximateLosses::SolidSlab][0][0] = "ApproxLoss_LRFD_SolidSlab_LowRelax_SI.gif";
      strLossEqnImage[0][lrfdApproximateLosses::SolidSlab][1][0] = "ApproxLoss_LRFD_SolidSlab_StressRel_SI.gif";
      strLossEqnImage[0][lrfdApproximateLosses::BoxBeam][0][0]   = "ApproxLoss_LRFD_BoxGirder_LowRelax_SI.gif";
      strLossEqnImage[0][lrfdApproximateLosses::BoxBeam][1][0]   = "ApproxLoss_LRFD_BoxGirder_StressRel_SI.gif";
      strLossEqnImage[0][lrfdApproximateLosses::SingleT][0][0]   = "ApproxLoss_LRFD_SingleT_LowRelax_SI.gif";
      strLossEqnImage[0][lrfdApproximateLosses::SingleT][1][0]   = "ApproxLoss_LRFD_SingleT_StressRel_SI.gif";

      strLossEqnImage[1][lrfdApproximateLosses::IBeam][0][0]     = "ApproxLoss_WSDOT_IBeam_LowRelax_SI.gif";
      strLossEqnImage[1][lrfdApproximateLosses::IBeam][1][0]     = "ApproxLoss_WSDOT_IBeam_StressRel_SI.gif";
      strLossEqnImage[1][lrfdApproximateLosses::UBeam][0][0]     = "ApproxLoss_WSDOT_UBeam_LowRelax_SI.gif";
      strLossEqnImage[1][lrfdApproximateLosses::UBeam][1][0]     = "ApproxLoss_WSDOT_UBeam_StressRel_SI.gif";
      strLossEqnImage[1][lrfdApproximateLosses::SolidSlab][0][0] = "ApproxLoss_LRFD_SolidSlab_LowRelax_SI.gif";
      strLossEqnImage[1][lrfdApproximateLosses::SolidSlab][1][0] = "ApproxLoss_LRFD_SolidSlab_StressRel_SI.gif";
      strLossEqnImage[1][lrfdApproximateLosses::BoxBeam][0][0]   = "ApproxLoss_LRFD_BoxGirder_LowRelax_SI.gif";
      strLossEqnImage[1][lrfdApproximateLosses::BoxBeam][1][0]   = "ApproxLoss_LRFD_BoxGirder_StressRel_SI.gif";
      strLossEqnImage[1][lrfdApproximateLosses::SingleT][0][0]   = "ApproxLoss_LRFD_SingleT_LowRelax_SI.gif";
      strLossEqnImage[1][lrfdApproximateLosses::SingleT][1][0]   = "ApproxLoss_LRFD_SingleT_StressRel_SI.gif";

      strLossEqnImage[0][lrfdApproximateLosses::IBeam][0][1]     = "ApproxLoss_LRFD_IBeam_LowRelax_US.gif";
      strLossEqnImage[0][lrfdApproximateLosses::IBeam][1][1]     = "ApproxLoss_LRFD_IBeam_StressRel_US.gif";
      strLossEqnImage[0][lrfdApproximateLosses::UBeam][0][1]     = "ApproxLoss_LRFD_UBeam_LowRelax_US.gif";
      strLossEqnImage[0][lrfdApproximateLosses::UBeam][1][1]     = "ApproxLoss_LRFD_UBeam_StressRel_US.gif";
      strLossEqnImage[0][lrfdApproximateLosses::SolidSlab][0][1] = "ApproxLoss_LRFD_SolidSlab_LowRelax_US.gif";
      strLossEqnImage[0][lrfdApproximateLosses::SolidSlab][1][1] = "ApproxLoss_LRFD_SolidSlab_StressRel_US.gif";
      strLossEqnImage[0][lrfdApproximateLosses::BoxBeam][0][1]   = "ApproxLoss_LRFD_BoxGirder_LowRelax_US.gif";
      strLossEqnImage[0][lrfdApproximateLosses::BoxBeam][1][1]   = "ApproxLoss_LRFD_BoxGirder_StressRel_US.gif";
      strLossEqnImage[0][lrfdApproximateLosses::SingleT][0][1]   = "ApproxLoss_LRFD_SingleT_LowRelax_US.gif";
      strLossEqnImage[0][lrfdApproximateLosses::SingleT][1][1]   = "ApproxLoss_LRFD_SingleT_StressRel_US.gif";

      strLossEqnImage[1][lrfdApproximateLosses::IBeam][0][1]     = "ApproxLoss_WSDOT_IBeam_LowRelax_US.gif";
      strLossEqnImage[1][lrfdApproximateLosses::IBeam][1][1]     = "ApproxLoss_WSDOT_IBeam_StressRel_US.gif";
      strLossEqnImage[1][lrfdApproximateLosses::UBeam][0][1]     = "ApproxLoss_WSDOT_UBeam_LowRelax_US.gif";
      strLossEqnImage[1][lrfdApproximateLosses::UBeam][1][1]     = "ApproxLoss_WSDOT_UBeam_StressRel_US.gif";
      strLossEqnImage[1][lrfdApproximateLosses::SolidSlab][0][1] = "ApproxLoss_LRFD_SolidSlab_LowRelax_US.gif";
      strLossEqnImage[1][lrfdApproximateLosses::SolidSlab][1][1] = "ApproxLoss_LRFD_SolidSlab_StressRel_US.gif";
      strLossEqnImage[1][lrfdApproximateLosses::BoxBeam][0][1]   = "ApproxLoss_LRFD_BoxGirder_LowRelax_US.gif";
      strLossEqnImage[1][lrfdApproximateLosses::BoxBeam][1][1]   = "ApproxLoss_LRFD_BoxGirder_StressRel_US.gif";
      strLossEqnImage[1][lrfdApproximateLosses::SingleT][0][1]   = "ApproxLoss_LRFD_SingleT_LowRelax_US.gif";
      strLossEqnImage[1][lrfdApproximateLosses::SingleT][1][1]   = "ApproxLoss_LRFD_SingleT_StressRel_US.gif";


      int method = (details.Method == LOSSES_WSDOT_LUMPSUM) ? 1 : 0;
      int beam = (int)details.ApproxLosses.GetBeamType();
      int strand = details.pLosses->GetStrandType() == matPsStrand::LowRelaxation ? 0 : 1;
      int units = (pDispUnits->GetUnitDisplayMode() == pgsTypes::umSI) ? 0 : 1;

      
      pParagraph = new rptParagraph;
      *pChapter << pParagraph;
      *pParagraph<< rptRcImage(strImagePath + strLossEqnImage[method][beam][strand][units]) << rptNewLine;

      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDispUnits->GetStressUnit(),          true );
      *pParagraph << RPT_FC << " = " << stress.SetValue(details.pLosses->GetFc() ) << rptNewLine;
      *pParagraph << "PPR = " << details.ApproxLosses.GetPPR() << rptNewLine;
      *pParagraph << symbol(DELTA) << Sub2("f","pLT") << " = " << stress.SetValue( details.pLosses->TimeDependentLosses() ) << rptNewLine;
   }
   else
   {
      if ( pDispUnits->GetUnitDisplayMode() == pgsTypes::umSI )
         *pParagraph<< rptRcImage(strImagePath + "LumpSumLossEquation2005_SI.gif") << rptNewLine;
      else
         *pParagraph<< rptRcImage(strImagePath + "LumpSumLossEquation2005_US.gif") << rptNewLine;

      rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(9,"");

      INIT_UV_PROTOTYPE( rptAreaUnitValue,    area,        pDispUnits->GetAreaUnit(),            false );
      INIT_UV_PROTOTYPE( rptStressUnitValue,  stress,      pDispUnits->GetStressUnit(),          false );

      rptRcScalar scalar;
      scalar.SetFormat( sysNumericFormatTool::Automatic );
      scalar.SetWidth(6);
      scalar.SetPrecision(2);

      *pParagraph << table << rptNewLine;
      (*table)(0,0) << COLHDR(RPT_FCI, rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*table)(0,1) << Sub2(symbol(gamma),"st");
      (*table)(0,2) << "Relative" << rptNewLine << "Humidity (%)";
      (*table)(0,3) << Sub2(symbol(gamma),"h");
      (*table)(0,4) << COLHDR(Sub2("f","pi"), rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*table)(0,5) << COLHDR(Sub2("A","ps"), rptAreaUnitTag, pDispUnits->GetAreaUnit() );
      (*table)(0,6) << COLHDR(Sub2("A","g"), rptAreaUnitTag, pDispUnits->GetAreaUnit() );
      (*table)(0,7) << COLHDR(symbol(DELTA) << Sub2("f","pR"), rptStressUnitTag, pDispUnits->GetStressUnit() );
      (*table)(0,8) << COLHDR(symbol(DELTA) << Sub2("f","pLT"), rptStressUnitTag, pDispUnits->GetStressUnit() );

      (*table)(1,0) << stress.SetValue( details.pLosses->GetFci() );
      (*table)(1,1) << scalar.SetValue( details.ApproxLosses2005.GetStrengthFactor() );
      (*table)(1,2) << details.pLosses->GetRelHumidity();
      (*table)(1,3) << scalar.SetValue( details.ApproxLosses2005.GetHumidityFactor() );
      (*table)(1,4) << stress.SetValue( details.ApproxLosses2005.GetFpi() );
      (*table)(1,5) << area.SetValue( details.pLosses->GetApsPermanent() );
      (*table)(1,6) << area.SetValue( details.pLosses->GetAg() );
      (*table)(1,7) << stress.SetValue( details.ApproxLosses2005.PermanentStrand_RelaxationLossesAtXfer() );
      (*table)(1,8) << stress.SetValue( details.pLosses->TimeDependentLosses() );
   }
}

void CPsLossEngineer::GetLossParameters(IBroker* pBroker,const pgsPointOfInterest& poi,const GDRCONFIG& config,
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
   Float64* pAd,
   Float64* ped,
   Float64* peperm,// eccentricity of the permanent strands on the non-composite section
   Float64* petemp,
   Float64* paps,  // area of one prestress strand
   Float64* pApsPerm,
   Float64* pApsTTS,
   Float64* pMdlg,
   Float64* pMadlg,
   Float64* pMsidl,
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
   GET_IFACE2(pBroker, IBridge,          pBridge );
   GET_IFACE2(pBroker, IStrandGeometry,  pStrandGeom );
   GET_IFACE2(pBroker, IGirderData,      pGirderData );
   GET_IFACE2(pBroker, ISectProp2,       pSectProp2 );
   GET_IFACE2(pBroker, IProductForces,   pProdForces );
   GET_IFACE2(pBroker, IEnvironment,     pEnv );
   GET_IFACE2(pBroker, IPointOfInterest, pIPOI );
   GET_IFACE2(pBroker, IBridgeMaterial,  pMaterial );
   GET_IFACE2(pBroker, ISpecification,   pSpec);
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();


   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   *pPjS = config.Pjack[pgsTypes::Straight];
   *pNs = config.Nstrands[pgsTypes::Straight];
   
   *pPjH = config.Pjack[pgsTypes::Harped];
   *pNh = config.Nstrands[pgsTypes::Harped];
   
   *pPjT = config.Pjack[pgsTypes::Temporary];
   *pNt  = config.Nstrands[pgsTypes::Temporary];

   const matPsStrand* pstrand = pGirderData->GetStrandMaterial(span,gdr);
   CHECK(pstrand);
   *pGrade = pstrand->GetGrade();
   *pType  = pstrand->GetType();
   *paps   = pstrand->GetNominalArea();

   *pGirderLength = pBridge->GetGirderLength( span, gdr);
   *pSpanLength   = pBridge->GetSpanLength( span, gdr );
   *pEndSize      = pBridge->GetGirderStartConnectionLength( span, gdr );

   Float64 nStrandsEffective;
   
   // eccentricity of the permanent strands
   *peperm = pStrandGeom->GetEccentricity( poi, config, false, &nStrandsEffective);

   // eccentricity of the temporary strands
   *petemp = pStrandGeom->GetTempEccentricity( poi, config, &nStrandsEffective);

   *pPerimeter = pSectProp2->GetPerimeter(poi);
   *pAg  = pSectProp2->GetAg( pgsTypes::BridgeSite1, poi );
   *pIg  = pSectProp2->GetIx( pgsTypes::BridgeSite1, poi );
   *pYbg = pSectProp2->GetYb( pgsTypes::BridgeSite1, poi );
   *pAc  = pSectProp2->GetAg( pgsTypes::BridgeSite2, poi, config.Fc );
   *pIc  = pSectProp2->GetIx( pgsTypes::BridgeSite2, poi, config.Fc );
   *pYbc = pSectProp2->GetYb( pgsTypes::BridgeSite2, poi, config.Fc );

   // area of deck
   if ( pBridge->IsCompositeDeck() )
       *pAd  = pSectProp2->GetGrossDeckArea( poi );
   else
       *pAd = 0.0;

   // eccentricity of deck
   *ped  = pSectProp2->GetYtGirder( pgsTypes::BridgeSite2, poi, config.Fc ) 
         + pBridge->GetStructuralSlabDepth(poi)/2;
   *ped *= -1;

   *pApsPerm = (*pNs+*pNh)*(*paps);
   *pApsTTS  = (*pNt)*(*paps);

   *pFci    = config.Fci;
   *pFc     = config.Fc;
   *pFcSlab = pMaterial->GetFcSlab();

   if ( config.bUserEci )
      *pEci = config.Eci;
   else
      *pEci = pMaterial->GetEconc(config.Fci,pMaterial->GetStrDensityGdr(span,gdr),pMaterial->GetK1Gdr(span,gdr));

   if( config.bUserEc )
      *pEc = config.Ec;
   else
      *pEc  = pMaterial->GetEconc(config.Fc, pMaterial->GetStrDensityGdr(span,gdr),pMaterial->GetK1Gdr(span,gdr));

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

   if ( poi.GetDistFromStart() < *pEndSize || *pEndSize + *pSpanLength < poi.GetDistFromStart() )
   {
      *pMadlg = 0;
   }
   else
   {
      *pMadlg = pProdForces->GetMoment( pgsTypes::BridgeSite1, pftSlab,      poi, bat ) +
                pProdForces->GetMoment( pgsTypes::BridgeSite1, pftDiaphragm, poi, bat ) + 
                pProdForces->GetMoment( pgsTypes::BridgeSite1, pftUserDC,    poi, bat ) +
                pProdForces->GetMoment( pgsTypes::BridgeSite1, pftUserDW,    poi, bat );
   }

   if ( m_bComputingLossesForDesign )
   {
      // get the additional moment caused by the difference in input and design "A" dimension
      double M = pProdForces->GetDesignSlabPadMomentAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi);
      *pMadlg += M;
   }

   if ( poi.GetDistFromStart() < *pEndSize || *pEndSize + *pSpanLength < poi.GetDistFromStart() )
   {
      *pMsidl = 0;
   }
   else
   {
      *pMsidl = pProdForces->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat ) +
                pProdForces->GetMoment( pgsTypes::BridgeSite2, pftSidewalk,       poi, bat ) +
                pProdForces->GetMoment( pgsTypes::BridgeSite2, pftUserDC,         poi, bat ) +
                pProdForces->GetMoment( pgsTypes::BridgeSite2, pftUserDW,         poi, bat );

      // only include overlay load if it is not a future overlay
      if ( !pBridge->IsFutureOverlay() )
         *pMsidl += pProdForces->GetMoment( pgsTypes::BridgeSite2, pftOverlay, poi, bat );
   }

   *prh = pEnv->GetRelHumidity();

   // get time to prestress transfer
   GET_IFACE2(pBroker,ILibrary,pLib);
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );
   *pti = pSpecEntry->GetXferTime();
   *pth = pSpecEntry->GetShippingTime();
   *ptd = pSpecEntry->GetCreepDuration2Max();
   *ptf = pSpecEntry->GetTotalCreepDuration();

   *pAslab = pSectProp2->GetTributaryDeckArea(poi);
   *pPslab = pSectProp2->GetTributaryFlangeWidth(poi);
   // *NOTE* Only the top portion of the slab is exposed to drying

   // Update the data members of the loss calculation object.  It will take care of the rest
   switch (config.TempStrandUsage)
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
