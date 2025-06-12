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

// WSDOTAgent.cpp : Implementation of DLL Exports.


#include "stdafx.h"
#include "resource.h"
#include <initguid.h>

#include "CLSID.h"
#include "dllmain.h"

#include "BridgeLinkCATID.h"
#include "PGSuperCatCom.h"
#include "PGSpliceCatCom.h"
#include <System\ComCatMgr.h>
#include "PGSComponentInfo.h"

// interfaces used in this DLL.... resolves symbols for the linker

#include <WBFLTools_i.c>
#include <WBFLUnitServer_i.c>
#include <EAF/EAFReportManager.h>
#include <EAF/EAFStatusCenter.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace/AnalysisResults.h>
#include <IFace/Bridge.h>
#include <IFace/Artifact.h>
#include <IFace/Project.h>
#include <IFace/PrestressForce.h>
#include <IFace/GirderHandling.h>
#include <IFace/GirderHandlingSpecCriteria.h>
#include <IFace/RatingSpecification.h>
#include <IFace/Intervals.h>
#include <IFace/DocumentType.h>
#include <IFace/Selection.h>
#include <IFace/Limits.h>
#include <IFace/PointOfInterest.h>

#include <Plugins\BeamFamilyCLSID.h>

