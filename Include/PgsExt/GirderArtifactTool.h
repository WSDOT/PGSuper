///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_GIRDERARTIFACTTOOL_H_
#define INCLUDED_PGSEXT_GIRDERARTIFACTTOOL_H_

#include <PgsExt\PgsExtExp.h>

#include <PgsExt\GirderArtifact.h>

#include <vector>

/*****************************************************************************

   GirderArtifactTools

   Tools for Artifact for a prestressed girder.


DESCRIPTION
   functions to work on artifacts

LOG
   rdp : 03.13.2009 : Created file
*****************************************************************************/

// At one time, these functions worked directly with the Reporting system, but the list
// of failures are needed by other subsystems and just strings work fine
typedef std::vector<std::_tstring> FailureList;
typedef FailureList::iterator    FailureListIterator;

void PGSEXTFUNC ListStressFailures(IBroker* pBroker, FailureList& rFailures, const pgsGirderArtifact* pGirderArtifact,bool referToDetailsReport);
void PGSEXTFUNC ListMomentCapacityFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls);
void PGSEXTFUNC ListVerticalShearFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls);
void PGSEXTFUNC ListHorizontalShearFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls);
void PGSEXTFUNC ListStirrupDetailingFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,pgsTypes::LimitState ls);
void PGSEXTFUNC ListDebondingFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact);
void PGSEXTFUNC ListSplittingZoneFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact);
void PGSEXTFUNC ListConfinementZoneFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact);
void PGSEXTFUNC ListVariousFailures(IBroker* pBroker,FailureList& rFailures,const pgsGirderArtifact* pGirderArtifact,bool referToDetails);

#endif // INCLUDED_PGSEXT_GIRDERARTIFACTTOOL_H_
