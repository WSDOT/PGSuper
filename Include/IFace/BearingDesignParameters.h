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




/*****************************************************************************
INTERFACE
   IBearingDesignParameters

   <<Summary here>>

DESCRIPTION
   <<Detailed description here>>
*****************************************************************************/


struct DESIGNPROPERTIES
{
	Float64 Fy{ WBFL::Units::ConvertToSysUnits(36, WBFL::Units::Measure::KSI) };///< steel yield strength
	Float64 Fth{ WBFL::Units::ConvertToSysUnits(24, WBFL::Units::Measure::KSI) };///< steel fatigue threshold
	Float64 Gmin;
	Float64 Gmax;
};

struct ROTATIONDETAILS
{
	Float64 girderRotation;
	Float64 diaphragmRotation;
	Float64 slabRotiaton;
	Float64 haunchRotation;
	Float64 maxRailingSystemRotation;
	Float64 minRailingSystemRotation;
	Float64 maxFutureOverlayRotation;
	Float64 minFutureOverlayRotation;
	Float64 maxDCrotation;
	Float64 maxDWrotation;
	Float64 maxDesignLLrotation;
	Float64 minDesignLLrotation;
	Float64 maxServiceIRotation;
};



// {D88670F0-3B83-11d2-8EC5-006097DF3C68}
DEFINE_GUID(IID_IBearingDesignParameters,
	0xD88670F0, 0x3B83, 0x11d2, 0x8E, 0xC5, 0x00, 0x60, 0x97, 0xDF, 0x3C, 0x68);
interface IBearingDesignParameters : IUnknown
{

	virtual void GetBearingRotationDetails(bool isFlexural, ROTATIONDETAILS* pDetails) const = 0;


};





