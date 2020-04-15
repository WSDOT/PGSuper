///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_PRINCIPALWEBSTRESS_H_
#define INCLUDED_IFACE_PRINCIPALWEBSTRESS_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#if !defined INCLUDED_PGSUPERTYPES_H_
#include <PGSuperTypes.h>
#endif

#if !defined INCLUDED_DETAILS_H_
#include <Details.h>
#endif

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsPointOfInterest;

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IPrincipalWebStress

   Interface to principal web stress information
*****************************************************************************/
// {179D8E05-2C51-4C32-AE11-71D489C4F804}
DEFINE_GUID(IID_IPrincipalWebStress,
   0x179d8e05, 0x2c51, 0x4c32, 0xae, 0x11, 0x71, 0xd4, 0x89, 0xc4, 0xf8, 0x4);
interface IPrincipalWebStress : IUnknown
{
   virtual const PRINCIPALSTRESSINWEBDETAILS* GetPrincipalWebStressDetails(const pgsPointOfInterest& poi) const = 0;
};

#endif // INCLUDED_IFACE_PRINCIPALWEBSTRESS_H_

