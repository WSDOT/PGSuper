///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include <psgLib\psgLibLib.h>
#include <PsgLib\LibraryManager.h>

// This special library manager is only used when loading PGSuper
// project data. Some library entry types have been simplified by
// removing data from them. When a library entry is in use, the
// removed data must be captured when it is loaded. This happens
// when the project file contains data in a format that is older
// than the current format. This class provides a place to store
// that data so that that project agent can move it to the correct
// location after the file is loaded. The library entries detect
// if this removed data needs to be stored by using RTTI on the 
// library manager.
class PSGLIBCLASS psgProjectLibraryManager : public psgLibraryManager
{
public:
   psgProjectLibraryManager();

   // Load Factors that were removed from the Spec Library entry version 41
   Float64 m_DCmin[6];   // index is one of pgsTypes::LimitState constants (except for CLLIM)
   Float64 m_DWmin[6];
   Float64 m_LLIMmin[6];
   Float64 m_DCmax[6];
   Float64 m_DWmax[6];
   Float64 m_LLIMmax[6];
};