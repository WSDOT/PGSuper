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

// {979E9DD4-9103-42ce-9601-1913EF69513E}
DEFINE_GUID(IID_IUpdateTemplates, 
0x979e9dd4, 0x9103, 0x42ce, 0x96, 0x1, 0x19, 0x13, 0xef, 0x69, 0x51, 0x3e);

/// @brief Interface that returns the state of the automatic template update feature.
/// The automatic template update feature is a "hidden" feature used by software developers
/// to programmatically modify templates when the automatic template update command is used (Ctrl+U for PGSuper and Ctrl+I for PGSplice)
class __declspec(uuid("{979E9DD4-9103-42ce-9601-1913EF69513E}")) IUpdateTemplates
{
public:
   /// @brief Returns true if application is in the process of automatically updating templates
   virtual bool UpdatingTemplates() = 0;
};
