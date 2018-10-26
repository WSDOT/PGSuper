///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// dllmain.h : Declaration of module class.

class CTxDOTAgentModule : public CAtlDllModuleT< CTxDOTAgentModule >
{
public :
	DECLARE_LIBID(LIBID_TxDOTAgentLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_TXDOTAGENT, "{B84B5AAB-22E8-41DB-B661-6238BC4D9EDD}")
};

extern class CTxDOTAgentModule _AtlModule;
