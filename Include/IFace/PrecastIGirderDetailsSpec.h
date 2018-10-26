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

#ifndef INCLUDED_IFACE_PRECASTIGIRDERDETAILSSPEC_H_
#define INCLUDED_IFACE_PRECASTIGIRDERDETAILSSPEC_H_

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

#include <PGSuperTypes.h>

// PROJECT INCLUDES
//
// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IPrecastIGirderDetailsSpec

   Interface to spec detailing requirements for precast I girders

DESCRIPTION
   Interface to spec detailing requirements for precast I girders
*****************************************************************************/
// {F3DD6462-D707-11d2-AD2C-00105A9AF985}
DEFINE_GUID(IID_IPrecastIGirderDetailsSpec, 
0xf3dd6462, 0xd707, 0x11d2, 0xad, 0x2c, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IPrecastIGirderDetailsSpec : IUnknown
{
   //------------------------------------------------------------------------
   // Minimum Top Flange thickness
   virtual Float64 GetMinTopFlangeThickness() const=0;

   //------------------------------------------------------------------------
   // Minimum web thickness
   virtual Float64 GetMinWebThickness() const=0;

   //------------------------------------------------------------------------
   // Minimum Bottom Flange thickness
   virtual Float64 GetMinBottomFlangeThickness() const=0;
};

#endif // INCLUDED_IFACE_PRECASTIGIRDERDETAILSSPEC_H_

