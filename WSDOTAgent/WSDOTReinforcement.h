#pragma once

#include <PsgLib\Keys.h>

namespace WBFL { namespace EAF { class Broker; }; };


#define STIRRUP_ERROR_NONE        0
#define STIRRUP_ERROR            -1
#define STIRRUP_ERROR_BARSIZE    -2
#define STIRRUP_ERROR_ZONES      -3
#define STIRRUP_ERROR_SYMMETRIC  -4
#define STIRRUP_ERROR_STARTZONE  -5
#define STIRRUP_ERROR_LASTZONE   -6
#define STIRRUP_ERROR_V6         -7


class CWSDOTReinforcement
{
public:
	int GetWSDOTReinforcementDetails(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CSegmentKey& segmentKey, CLSID& familyCLSID,
		Float64* pz1Spacing, Float64* pz1Length, Float64* pz2Spacing, Float64* pz2Length, Float64* pz3Spacing, Float64* pz3Length) const;

};