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


#include "psgLibLib.h"

class rptParagraph;
interface IEAFDisplayUnits;

struct PSGLIBCLASS TensionStressLimit
{
   Float64 Coefficient = 0.0; ///< Coefficient on (lambda)sqrt(f'c)
   bool    bHasMaxValue = false;
   Float64 MaxValue = 0.0;


   bool operator==(const TensionStressLimit& other) const;

   void Report(rptParagraph* pPara, IEAFDisplayUnits* pDisplayUnits) const;

   void Save(LPCTSTR strUnitName,WBFL::System::IStructuredSave* pSave) const;
   void Load(LPCTSTR strUnitName, WBFL::System::IStructuredLoad* pLoad);
};

