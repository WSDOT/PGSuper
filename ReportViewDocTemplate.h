///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "CountedMultiDocTemplate.h"

class CReportViewDocTemplate :
   public CCountedMultiDocTemplate
{
public:
   CReportViewDocTemplate(UINT nIDResource,
                            CRuntimeClass* pDocClass,
                            CRuntimeClass* pFrameClass,
                            CRuntimeClass* pViewClass,
                            int maxViewCount = -1);
   ~CReportViewDocTemplate(void);

   CollectionIndexType GetReportIndex();
   void SetReportIndex(CollectionIndexType rptIdx);

   bool PromptForReportSpecification();
   void PromptForReportSpecification(bool bPrompt);

private:
   // These two members are temporary. They only contain valid data during
   // the creation of the view. The CPGSuperApp::GetReportViewTemplate sets these
   // parameters and then the CReportView gets this template and reads the values
   // to help with the view creation. These data members are invalid outside of
   // that scope

   CollectionIndexType m_RptIdx;
   bool m_bPrompt;
};
