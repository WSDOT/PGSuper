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

// PGSpliceGraphReporterImp.cpp : Implementation of CPGSpliceGrapherImp
#include "stdafx.h"
#include <IFace\Tools.h>
#include "PGSpliceGrapherImp.h"


//CPGSpliceGrapherImp::InitGraphBuilders
//
//Initialize graph builders at project load time
//
//Initialize graph builders at project load time.  When a project
//file is OPENed, this method is called as part of the opening
//events sequence.
//
//Returns S_OK if successful; otherwise appropriate HRESULT value
//is returned.

HRESULT CPGSpliceGrapherImp::InitGraphBuilders()
{
   InitCommonGraphBuilders(m_pBroker);
   return S_OK;
}

bool CPGSpliceGrapherImp::Init()
{
   EAF_AGENT_INIT;

   return InitGraphBuilders();
}

CLSID CPGSpliceGrapherImp::GetCLSID() const
{
   return CLSID_PGSpliceGraphingAgent;
}
