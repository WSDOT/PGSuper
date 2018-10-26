///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
// 4500 3rd AVE SE - P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_IFACE_FILE_H_
#define INCLUDED_IFACE_FILE_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-2003
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//
#include <WbflTypes.h>

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
   IFile

   Interface for file information

DESCRIPTION
   Interface for file information
*****************************************************************************/
// {A69359E9-483D-40ec-96D7-F02E4E82C65E}
DEFINE_GUID(IID_IFile, 
0xa69359e9, 0x483d, 0x40ec, 0x96, 0xd7, 0xf0, 0x2e, 0x4e, 0x82, 0xc6, 0x5e);
interface IFile : IUnknown
{
   // example: c:\my documents\pgsuper.pgs
   virtual std::string GetFileName() = 0;  // returns pgsuper.pgs
   virtual std::string GetFileTitle() = 0; // returns pgsuper
   virtual std::string GetFilePath() = 0;  // c:\my documents\pgsuper.pgs
   virtual std::string GetFileRoot() = 0;  // c:\my documents
};

#endif // INCLUDED_IFACE_FILE_H_
