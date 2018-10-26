///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_PRESTRESS_H_
#define INCLUDED_IFACE_PRESTRESS_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#include <Details.h>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsPointOfInterest;
class matPsStrand;
class rptChapter;
interface IDisplayUnits;

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IPrestressForce

   Interface to prestress force and stress information

DESCRIPTION
   Interface to prestress force and stress information.  This is computed
   information and not the raw input data.
*****************************************************************************/
// {381E19E0-6E82-11d2-8EEB-006097DF3C68}
DEFINE_GUID(IID_IPrestressForce, 
0x381e19e0, 0x6e82, 0x11d2, 0x8e, 0xeb, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IPrestressForce : IUnknown
{
   virtual Float64 GetPjackMax(SpanIndexType span,GirderIndexType gdr,StrandIndexType nStrands) = 0;
   virtual Float64 GetPjackMax(SpanIndexType span,GirderIndexType gdr,const matPsStrand& strand,StrandIndexType nStrands) = 0;

   virtual Float64 GetXferLength(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetDevLength(const pgsPointOfInterest& poi,bool bDebonded) = 0;
   virtual STRANDDEVLENGTHDETAILS GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded) = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,StrandIndexType strandIdx,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe) = 0;
   virtual Float64 GetStrandBondFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,StrandIndexType strandIdx,pgsTypes::StrandType strandType,Float64 fps,Float64 fpe) = 0;

   virtual Float64 GetHoldDownForce(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual Float64 GetHoldDownForce(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config) = 0;

   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage) = 0;
   virtual Float64 GetPrestressForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage) = 0;

   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage) = 0;
   virtual Float64 GetPrestressForcePerStrand(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage) = 0;

   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,pgsTypes::LossStage lossStage) = 0;
   virtual Float64 GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::LossStage lossStage) = 0;

   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,pgsTypes::LossStage lossStage) = 0;
   virtual Float64 GetVertHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::LossStage lossStage) = 0;

   virtual Float64 GetStrandForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage) = 0;
   virtual Float64 GetStrandForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config,pgsTypes::LossStage lossStage) = 0;

   virtual Float64 GetStrandStress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LossStage lossStage) = 0;
   virtual Float64 GetStrandStress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config,pgsTypes::LossStage lossStage) = 0;
};

/*****************************************************************************
INTERFACE
   ILosses

   Interface to get losses.

DESCRIPTION
   Interface to get losses.
*****************************************************************************/
// {03D91150-6DBB-11d2-8EE9-006097DF3C68}
DEFINE_GUID(IID_ILosses, 
0x3d91150, 0x6dbb, 0x11d2, 0x8e, 0xe9, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface ILosses : IUnknown
{
   // losses based on current input
   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetBeforeXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetAfterXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetLiftingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetShippingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetAfterTemporaryStrandInstallationLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetBeforeTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetAfterTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetDeckPlacementLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetFinal(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) = 0;
   virtual LOSSDETAILS GetLossDetails(const pgsPointOfInterest& poi) = 0;

   // losses based on a girder configuration and slab offset
   virtual Float64 GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual Float64 GetBeforeXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual Float64 GetAfterXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual Float64 GetLiftingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual Float64 GetShippingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual Float64 GetAfterTemporaryStrandInstallationLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual Float64 GetBeforeTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual Float64 GetAfterTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual Float64 GetDeckPlacementLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual Float64 GetFinal(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) = 0;
   virtual LOSSDETAILS GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;
   virtual void ClearDesignLosses() = 0;

   virtual void ReportLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnit) = 0;
};

#endif // INCLUDED_IFACE_PRESTRESS_H_

