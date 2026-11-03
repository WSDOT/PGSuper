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

#include "stdafx.h"
#include <AgentTools.h>
#include "resource.h"
#include "XBeamCutLocation.h"

#include <IFace\Selection.h>

#include <PsgLib\GirderLabel.h>
#include <PsgLib\BridgeDescription2.h>




/////////////////////////////////////////////////////////////////////////////
// CXBeamCutLocation


CXBeamCutLocation::CXBeamCutLocation()
{
   m_Xmin = 0;
   m_CutLocation = 2;
   m_Xmax = 5;
}

CXBeamCutLocation::CXBeamCutLocation(Float64 xMin, Float64 xLoc, Float64 xMax)
{
   m_Xmin = xMin;
   m_CutLocation = xLoc;
   m_Xmax = xMax;
}

CXBeamCutLocation::~CXBeamCutLocation()
{
}

Float64 CXBeamCutLocation::GetCurrentCutLocation()
{
   return m_CutLocation;
}

Float64 CXBeamCutLocation::GetMinCutLocation()
{
   return m_Xmin;
}

Float64 CXBeamCutLocation::GetMaxCutLocation()
{
   return m_Xmax;
}

void CXBeamCutLocation::UpdateSectionCutExtents()
{

}

