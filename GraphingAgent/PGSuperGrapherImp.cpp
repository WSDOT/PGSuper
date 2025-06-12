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

// PGSuperGrapherImp.cpp : Implementation of CPGSuperGrapherImp
#include "stdafx.h"
#include <IFace\Tools.h>
#include "PGSuperGrapherImp.h"


//CPGSuperGrapherImp::InitGraphBuilders
//
//Initialize graph builders at project load time
//
//Initialize graph builders at project load time.  When a project
//file is OPENed, this method is called as part of the opening
//events sequence.
//
//Returns S_OK if successful; otherwise appropriate HRESULT value
//is returned.

bool CPGSuperGrapherImp::InitGraphBuilders()
{
   return InitCommonGraphBuilders(m_pBroker);
}

bool CPGSuperGrapherImp::Init()
{
   EAF_AGENT_INIT;

   return InitGraphBuilders();
}

CLSID CPGSuperGrapherImp::GetCLSID() const
{
   return CLSID_PGSuperGraphingAgent;
}
