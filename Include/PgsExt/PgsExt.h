///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include <EAF/AutoProgress.h>
#include <PgsExt\ConstructabilityArtifact.h>
#include <PgsExt\GirderDesignArtifact.h>
#include <PgsExt\SegmentDesignArtifact.h>
#include <PgsExt\FlexuralCapacityArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\HoldDownForceArtifact.h>
#include <PsgLib\PierData2.h>
#include <PsgLib\SpanData2.h>
#include <PsgLib\BridgeDescription2.h>
#include <PgsExt\PoiArtifactKey.h>
#include <PgsExt\PoiMgr.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <PsgLib\HandlingData.h>
#include <PgsExt\StrandSlopeArtifact.h>
#include <PgsExt\StrandStressArtifact.h>
#include <PgsExt\PrincipalTensionStressArtifact.h>
#include <PsgLib\GirderLabel.h>
#include <PgsExt\CapacityToDemand.h>
#include <PgsExt\ReinforcementFatigueArtifact.h>
