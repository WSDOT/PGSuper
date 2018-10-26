///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_VERSIONINFO_H_
#define INCLUDED_IFACE_VERSIONINFO_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//

/*****************************************************************************
INTERFACE
   IVersionInfo

   Interface for getting version information

DESCRIPTION
   Interface for getting version information
*****************************************************************************/
// {70DD414A-B493-11d2-88BB-006097C68A9C}
DEFINE_GUID(IID_IVersionInfo, 
0x70dd414a, 0xb493, 0x11d2, 0x88, 0xbb, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IVersionInfo : IUnknown
{
   // Returns the version string that should be displayed in reports, etc
   // This string has the format
   // Version a.b.c.d - Built on MM/YYYY
   // The build number (.d) is omitted if bIncludeBuildNumber is false
   virtual CString GetVersionString(bool bIncludeBuildNumber = false) = 0;

   // Returns the version number as a string in the format a.b.c.d
   // The build number (.d) is omitted if bIncludeBuildNumber is false
   virtual CString GetVersion(bool bIncludeBuildNumber = false) = 0;
};
#endif // INCLUDED_IFACE_VERSIONINFO_H_
