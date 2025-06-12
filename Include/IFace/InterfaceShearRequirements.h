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

#pragma once

// SYSTEM INCLUDES
//

#include <PGSuperTypes.h>

/*****************************************************************************
INTERFACE
   IInterfaceShearRequirements

   Interface for interface shear requirements
*****************************************************************************/
// {7555754F-0F6D-4ba8-8049-8E111A008427}
DEFINE_GUID(IID_IInterfaceShearRequirements, 
0x7555754f, 0xf6d, 0x4ba8, 0x80, 0x49, 0x8e, 0x11, 0x1a, 0x0, 0x84, 0x27);
class IInterfaceShearRequirements
{
public:
   //------------------------------------------------------------------------
   // returns the method for computing shear flow
   virtual pgsTypes::ShearFlowMethod GetShearFlowMethod() const = 0;

   //------------------------------------------------------------------------
   // returns the maximum longitudinal center-to-center spacing of
   // nonwelded interface shear connectors (LRFD 5.7.4.5 (pre2017: 5.8.4.2))
   virtual Float64 GetMaxShearConnectorSpacing(const pgsPointOfInterest& poi) const = 0;
};
