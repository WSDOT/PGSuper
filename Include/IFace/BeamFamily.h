///////////////////////////////////////////////////////////////////////
// PGSuper Beam Family
// Copyright © 1999-2010  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This library is a part of the Washington Bridge Foundation Libraries
// and was developed as part of the Alternate Route Project
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the Alternate Route Library Open Source License as published by 
// the Washington State Department of Transportation, Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but is distributed 
// AS IS, WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
// or FITNESS FOR A PARTICULAR PURPOSE. See the Alternate Route Library Open Source 
// License for more details.
//
// You should have received a copy of the Alternate Route Library Open Source License 
// along with this program; if not, write to the Washington State Department of 
// Transportation, Bridge and Structures Office, P.O. Box  47340, 
// Olympia, WA 98503, USA or e-mail Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// The intent of licensing this interface with the ARLOSL is to provide
// third party developers a method of developing proprietary plug-ins
// to the software.
// 
// Any changes made to the interfaces defined in this file are
// are subject to the terms of the Alternate Route Library Open Source License.
//
// Components that implement the interfaces defined in this file are
// governed by the terms and conditions deemed appropriate by legal 
// copyright holder of said software.
///////////////////////////////////////////////////////////////////////

#pragma once

// forward declaration
interface IBeamFactory;

/*****************************************************************************
INTERFACE
   IBeamFamily

   Interface for creating a generic beam family.

DESCRIPTION
   A beam family is a general classification of a type of precast beam.
   Examples of beam families are I-Beam, U-Beam, and Slab
*****************************************************************************/
// {5F21B512-6AA0-4884-8ACC-86704ED51506}
DEFINE_GUID(IID_IBeamFamily, 
0x5f21b512, 0x6aa0, 0x4884, 0x8a, 0xcc, 0x86, 0x70, 0x4e, 0xd5, 0x15, 0x6);
interface IBeamFamily : IUnknown
{
   //---------------------------------------------------------------------------------
   // Return the family name
   virtual CString GetName() = 0;

   //---------------------------------------------------------------------------------
   // Returns a vector of beam factory names
   virtual std::vector<CString> GetFactoryNames() = 0;

   //---------------------------------------------------------------------------------
   // Returns the factory CLSID
   virtual CLSID GetFactoryCLSID(LPCTSTR strName) = 0;

   //---------------------------------------------------------------------------------
   // Creates a beam factory
   virtual HRESULT CreateFactory(LPCTSTR strName,IBeamFactory** ppFactory) = 0;
};


