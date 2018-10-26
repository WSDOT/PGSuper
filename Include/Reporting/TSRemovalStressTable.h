///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include "ReportNotes.h"

interface IEAFDisplayUnits;
interface IRatingSpecification;

class REPORTINGCLASS CTSRemovalStressTable
{
public:
   CTSRemovalStressTable();
   CTSRemovalStressTable(const CTSRemovalStressTable& rOther);
   virtual ~CTSRemovalStressTable();

   CTSRemovalStressTable& operator = (const CTSRemovalStressTable& rOther);

   void Build(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IEAFDisplayUnits* pDisplayUnits,bool bGirderStresses) const;

protected:
   void MakeCopy(const CTSRemovalStressTable& rOther);
   virtual void MakeAssignment(const CTSRemovalStressTable& rOther);
};
