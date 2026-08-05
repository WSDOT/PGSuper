///////////////////////////////////////////////////////////////////////
// PGSuper Beam Family
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include <EAF/ComponentObject.h>

namespace PGS
{
   namespace Beams
   {
      class BeamFactory;

      /// @brief A general classification of a type of precast beam - e.g. I-Beam, U-Beam, or Slab.
      /// A BeamFamily doesn't itself describe a cross section; it's a named collection of one or
      /// more BeamFactory implementations (the actual named shapes within that classification,
      /// e.g. "Precast I-Beam" and "Nebraska NU Girder" within the I-Beam family), each registered
      /// under the CATID this family reports from its own component category (see
      /// \ref creating_a_beam_type "Creating a Beam Type"). Concrete families normally derive from
      /// `PGS::Beams::BeamFamilyImpl` (`Include\Beams\Helper.h`) rather than implementing this
      /// interface directly.
      class BeamFamily : public WBFL::EAF::ComponentObject
      {
      public:

         // Return the family name
         virtual CString GetName() const = 0;

         // Causes the list of factory names to be read from the registry
         virtual void RefreshFactoryList() = 0;

         // Returns a vector of beam factory names
         virtual const std::vector<CString>& GetFactoryNames() const = 0;

         // Returns the factory CLSID
         virtual CLSID GetFactoryCLSID(LPCTSTR strName) const = 0;

         // Creates a beam factory
         virtual std::shared_ptr<BeamFactory> CreateFactory(LPCTSTR strName) const = 0;
      };
   };
};
