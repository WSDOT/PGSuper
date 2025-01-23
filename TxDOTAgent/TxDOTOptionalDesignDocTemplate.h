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

#include <EAF\EAFDocTemplate.h>

class CTxDOTOptionalDesignDocTemplate : public CEAFDocTemplate
{
public:
   CTxDOTOptionalDesignDocTemplate(UINT nIDResource,
                                   IEAFCommandCallback* pCallback,
                                   CRuntimeClass* pDocClass,
                                   CRuntimeClass* pFrameClass,
                                   CRuntimeClass* pViewClass,
                                   HMENU hSharedMenu = nullptr,
                                   int maxViewCount = -1);

   virtual void SetPlugin(IEAFAppPlugin* pPlugin) override;

   // returns the string that goes at teh bottom of the New dialog
   virtual CString GetTemplateGroupItemDescription(const CEAFTemplateItem* pItem) const override;

   DECLARE_DYNAMIC(CTxDOTOptionalDesignDocTemplate)

   void LoadTemplateInformation();

protected:
   void FindInFolder(LPCTSTR strPath,CEAFTemplateGroup* pGroup,HICON defaultIcon);
   void FindTemplateFiles(LPCTSTR strPath,CEAFTemplateGroup* pGroup,HICON folderIcon);
};
