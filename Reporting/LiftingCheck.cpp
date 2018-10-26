///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <IFace\Artifact.h>

#include <Reporting\LiftingCheck.h>
#include <Reporting\ReportNotes.h>

#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Project.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\CapacityToDemand.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <Stability\Stability.h>

#include <limits>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLiftingCheck
****************************************************************************/
CLiftingCheck::CLiftingCheck()
{
}

CLiftingCheck::CLiftingCheck(const CLiftingCheck& rOther)
{
   MakeCopy(rOther);
}

CLiftingCheck::~CLiftingCheck()
{
}

CLiftingCheck& CLiftingCheck::operator= (const CLiftingCheck& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CLiftingCheck::Build(rptChapter* pChapter,
                          IBroker* pBroker,const CGirderKey& girderKey,
                          IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IArtifact,pArtifacts);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GET_IFACE2(pBroker,IGirder,pGirder);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey thisSegmentKey(girderKey,segIdx);
      const stbLiftingCheckArtifact* pArtifact = pArtifacts->GetLiftingCheckArtifact(thisSegmentKey);
      const stbIGirder* pStabilityModel = pGirder->GetSegmentStabilityModel(thisSegmentKey);
      const stbILiftingStabilityProblem* pStabilityProblem = pGirder->GetSegmentLiftingStabilityProblem(thisSegmentKey);
      Float64 Ll, Lr;
      pStabilityProblem->GetSupportLocations(&Ll,&Lr);
      stbLiftingStabilityReporter reporter;
      reporter.BuildSpecCheckChapter(pStabilityModel,pStabilityProblem,pArtifact,pChapter,_T("Location from<BR/>Left Pick Point"),Ll);
   } // next segment
}

void CLiftingCheck::MakeCopy(const CLiftingCheck& rOther)
{
   // Add copy code here...
}

void CLiftingCheck::MakeAssignment(const CLiftingCheck& rOther)
{
   MakeCopy( rOther );
}
