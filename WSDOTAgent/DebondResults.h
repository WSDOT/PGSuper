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