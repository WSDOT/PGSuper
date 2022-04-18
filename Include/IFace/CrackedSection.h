///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

class pgsPointOfInterest;
#include <Details.h>

/*****************************************************************************
INTERFACE
   ICrackedSection

DESCRIPTION
   Interface to get cracked section information.
*****************************************************************************/
// {CD48333F-E8B8-4025-89C4-86BA05E60121}
DEFINE_GUID(IID_ICrackedSection, 
0xcd48333f, 0xe8b8, 0x4025, 0x89, 0xc4, 0x86, 0xba, 0x5, 0xe6, 0x1, 0x21);
interface ICrackedSection : IUnknown
{
   virtual Float64 GetIcr(const pgsPointOfInterest& poi,bool bPositiveMoment) const = 0;
   virtual const CRACKEDSECTIONDETAILS* GetCrackedSectionDetails(const pgsPointOfInterest& poi, bool bPositiveMoment) const = 0;
   virtual std::vector<const CRACKEDSECTIONDETAILS*> GetCrackedSectionDetails(const PoiList& vPoi,bool bPositiveMoment) const = 0;
};
