///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

/// @brief Interface used to determine the document type that is currently open.
/// @description This interface should not generally be used since the calling code
/// is making assumptions about the capabilities of the software based on document type
/// Use of this interface should be extremely limited.
// {BEA4D31A-3C91-4d6e-AB8B-F363106E3AB3}
DEFINE_GUID(IID_IDocumentType,
   0xbea4d31a, 0x3c91, 0x4d6e, 0xab, 0x8b, 0xf3, 0x63, 0x10, 0x6e, 0x3a, 0xb3);
struct __declspec(uuid("{BEA4D31A-3C91-4d6e-AB8B-F363106E3AB3}")) IDocumentType;
interface IDocumentType : IUnknown
{
   /// @brief Returns true if the current document is a PGSuper project
   virtual bool IsPGSuperDocument() const = 0;

   /// @brief Returns true if the current document is a PGSplice project
   virtual bool IsPGSpliceDocument() const = 0;
};

/// @brief Interfaced to get the document unit system unit server
// {A3ABCA05-C3A3-4797-904A-F4D6E6147A92}
DEFINE_GUID(IID_IDocumentUnitSystem, 
0xa3abca05, 0xc3a3, 0x4797, 0x90, 0x4a, 0xf4, 0xd6, 0xe6, 0x14, 0x7a, 0x92);
struct __declspec(uuid("{A3ABCA05-C3A3-4797-904A-F4D6E6147A92}")) IDocumentUnitSystem;
interface IDocumentUnitSystem : IUnknown
{
   /// @brief Gets the IUnitServer object for the current document
   virtual void GetUnitServer(IUnitServer** ppUnitServer) = 0;
};
