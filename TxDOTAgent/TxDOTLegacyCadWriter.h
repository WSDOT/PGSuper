///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// PROJECT INCLUDES
//
#include <IFace\Bridge.h>

// Utility class for structuring debond data
#include <PgsExt\DebondUtil.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
// Utility constants, stuff for TxDOT CAD export
#define CAD_DELIM	_T(" ")
#define CAD_SPACE	_T(" ")

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   TxDOTCadWriter

   Utility class for writing TxDOT CAD file

DESCRIPTION
   Utility class for writing TxDOT CAD file

LOG
   rdp : 04.09.2009 : Created file
*****************************************************************************/

// Main External function that write the file
int TxDOT_WriteLegacyCADDataToFile(CString& filePath, IBroker* pBroker, const std::vector<CGirderKey>& girderKeys);

