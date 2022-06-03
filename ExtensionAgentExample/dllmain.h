///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
// Copyright © 1999-2022  Washington State Department of Transportation
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

// dllmain.h : Declaration of module class.

class CExtensionAgentExampleModule : public CAtlDllModuleT< CExtensionAgentExampleModule >
{
public :
	DECLARE_LIBID(LIBID_ExtensionAgentExampleLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_EXTENSIONAGENTEXAMPLE, "{9AF59E4A-E471-479E-B6AF-888921EFDEAE}")
};

extern class CExtensionAgentExampleModule _AtlModule;
