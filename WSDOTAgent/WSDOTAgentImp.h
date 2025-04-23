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

// WSDOTAgentImp.h : Declaration of the CWSDOTAgentImp

#pragma once

#include "CLSID.h"
#include <EAF\Agent.h>

class CWSDOTAgentImp : public WBFL::EAF::Agent
{
public:
	CWSDOTAgentImp()
	{
	}

// Agent
public:
	bool RegInterfaces() override;
	bool Init() override;
	bool ShutDown() override;
   CLSID GetCLSID() const override;

private:
#pragma Reminder("WORKING HERE - Removing COM")
	// Consider using something like this interface cache
	// Must be careful that the broker is a weak pointer
	// so there isnt circular references between broker and agent
	// This could probably be implemented on the Agent class an then
	// inhertied by all sub-classes
	//DECLARE_EAF_INTERFACE_CACHE;
   DECLARE_LOGFILE;
};
