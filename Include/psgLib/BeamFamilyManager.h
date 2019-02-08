///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include "psgLibLib.h"

#include <IFace\BeamFamily.h>

/////////////////////////////////////////////////////////////////////////
// CBeamFamilyManager
//
// This class encapulates the discovery and creation of Beam Family objects
//

class PSGLIBCLASS CBeamFamilyManager
{
public:
   static HRESULT Init(CATID catid);

   static std::vector<CString> GetBeamFamilyNames();
   static std::vector<CString> GetBeamFamilyNames(CATID catid);
   static HRESULT GetBeamFamily(LPCTSTR strName,IBeamFamily** ppFamily);
   static CLSID GetBeamFamilyCLSID(LPCTSTR strName);
   static void Reset();
   static void UpdateFactories();

private:
   typedef std::map<CString,CLSID> BeamContainer;
   typedef std::map<CATID,BeamContainer> FamilyContainer;
   static FamilyContainer m_Families;
};
