///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <PgsExt\PgsExtExp.h>
class CProjectAgentImp; // Only one privy to private conversion data

class PGSEXTCLASS CDebondData
{
public:
   GridIndexType strandTypeGridIdx; // Index of debonded strand in GirderLibraryEntry strand grid
   Float64 Length1; // debond length at left end of girder
   Float64 Length2; // debond length at right end of girder

   CDebondData():
      strandTypeGridIdx(INVALID_INDEX), Length1(0.0), Length2(0.0)
   {;}

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   bool operator==(const CDebondData& rOther) const; 
   bool operator!=(const CDebondData& rOther) const;

friend CProjectAgentImp; // Only one privy to private conversion data
private:
   bool needsConversion; // Previous versions had two indices for strand data this can only be converted
                         // after we have access to the girder library
};
