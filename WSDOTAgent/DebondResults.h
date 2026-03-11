///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include <PsgLib\Keys.h>

namespace WBFL { namespace EAF { class Broker; }; };


#define DEBOND_ERROR_NONE        0
#define DEBOND_ERROR_SYMMETRIC   -1

class CDebondResults
{
public:

	struct DebondInformation
	{
		std::vector<StrandIndexType> Strands;
		Float64 Length;
	};

	int GetDebondDetails(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey, std::vector<DebondInformation>& debondInfo) const;

};