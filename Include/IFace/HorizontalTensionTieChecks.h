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

class pgsHorizontalTieForceArtifact;
class pgsGirderArtifact;

// {7A373E6A-2EA1-4343-A240-09A93F38F420}
DEFINE_GUID(IID_IHorizontalTensionTieChecks,
   0x7a373e6a, 0x2ea1, 0x4343, 0xa2, 0x40, 0x9, 0xa9, 0x3f, 0x38, 0xf4, 0x20);

class IHorizontalTensionTieChecks
{
public:
   virtual void CheckHorizontalTensionTieForce(const CGirderKey& girderKey, pgsGirderArtifact* pGdrArtifact) const = 0;
   virtual void ReportHorizontalTensionTieForceChecks(const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const = 0;
   virtual void ReportHorizontalTensionTieForceCheckDetails(const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const = 0;
};

