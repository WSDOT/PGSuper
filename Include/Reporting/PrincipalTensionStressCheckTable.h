///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;
class pgsGirderArtifact;
class pgsPrincipalTensionStressArtifact;

class REPORTINGCLASS CPrincipalTensionStressCheckTable
{
public:
   CPrincipalTensionStressCheckTable();
   CPrincipalTensionStressCheckTable(const CPrincipalTensionStressCheckTable& rOther);
   virtual ~CPrincipalTensionStressCheckTable();

   CPrincipalTensionStressCheckTable& operator = (const CPrincipalTensionStressCheckTable& rOther);

   virtual void Build(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits) const;

protected:
   void BuildTable(rptChapter* pChapter, IBroker* pBroker, const pgsGirderArtifact* pGirderArtifact, IEAFDisplayUnits* pDisplayUnits) const;
   void MakeCopy(const CPrincipalTensionStressCheckTable& rOther);
   void MakeAssignment(const CPrincipalTensionStressCheckTable& rOther);
};
