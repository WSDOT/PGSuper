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

// TxDOTAgentImp.h : Declaration of the CTxDOTAgentImp

#pragma once
#include <EAF\Agent.h>

#include "CLSID.h"

#include <EAF\EAFInterfaceCache.h>
#include <EAF\EAFUIIntegration.h>

#include <IFace\TestFileExport.h>

#include <PgsExt\GirderDesignArtifact.h>
#include "TxDOTCommandLineInfo.h"


// CTxDOTAgentImp

class CTxDOTAgentImp : public WBFL::EAF::Agent,
   public IEAFProcessCommandLine
{
public:
	CTxDOTAgentImp()
	{
	}

// Agent
public:
   bool RegInterfaces() override;
   bool Init() override;
   bool Reset() override;
   bool ShutDown() override;
   CLSID GetCLSID() const override;


// IEAFProcessCommandLine
public:
   BOOL ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo) override;

protected:
   void ProcessTOGAReport(const CTxDOTCommandLineInfo& rCmdInfo);
   bool DoTOGAReport(const CString& outputFileName, const CTxDOTCommandLineInfo& txInfo);

private:
   //DECLARE_EAF_AGENT_DATA;
   DECLARE_LOGFILE;
};
