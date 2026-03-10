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

/// @brief
// {70C00171-9F5B-4ac4-B776-2A2DF5B08803}
DEFINE_GUID(IID_IDesign, 
0x70c00171, 0x9f5b, 0x4ac4, 0xb7, 0x76, 0x2a, 0x2d, 0xf5, 0xb0, 0x88, 0x3);
/// @brief Interface to activate the interactive design process.
class __declspec(uuid("{70C00171-9F5B-4ac4-B776-2A2DF5B08803}")) IDesign
{
public:
   /// @brief Perform a girder design - only valid with PGSuper projects.
   /// @param bPrompt If true, the user is prompted for design options, otherwise design starts using the last used options
   /// @param bDesignSlabOffset If true, the slab offset (haunch) is designed
   /// @param girderKey The girder to be designed
   virtual void DesignGirder(bool bPrompt,bool bDesignSlabOffset,const CGirderKey& girderKey) = 0;

   /// @brief Performs a haunch design - only valid with Time-step analysis (PGSplice)
   /// Always brings up dialog prompt. 
   /// @return true if design is done, otherwise false (if design is cancelled)
   virtual bool DesignHaunch(const CGirderKey& girderKey) = 0;
};
