///////////////////////////////////////////////////////////////////////
// PGSplice - Precast Post-tensioned Spliced Girder Design and Analysis
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

#ifndef INCLUDED_DEBONDCHECKTABLE_H_
#define INCLUDED_DEBONDCHECKTABLE_H_

#include <Reporting\ReportingExp.h>

class IEAFDisplayUnits;
class pgsGirderArtifact;
class pgsDebondArtifact;


/*****************************************************************************
CLASS 
   CDebondCheckTable

   Encapsulates the construction of the debond check table.


DESCRIPTION
   Encapsulates the construction of the debond check table.

LOG
   rab : 06.11.2003 : Created file
*****************************************************************************/

class REPORTINGCLASS CDebondCheckTable
{
public:
   CDebondCheckTable();
   virtual ~CDebondCheckTable();

   void Build(rptChapter* pChapter, std::shared_ptr<WBFL::EAF::Broker> pBroker,const pgsGirderArtifact* pGirderArtifact,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;

private:
   CDebondCheckTable(const CDebondCheckTable& rOther) = delete;
   CDebondCheckTable& operator=(const CDebondCheckTable&) = delete;

   rptRcTable* Build1(const pgsDebondArtifact* pDebondArtifact, bool bAfter8thEdition, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
   rptRcTable* Build2(const pgsDebondArtifact* pDebondArtifact, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
};

#endif // INCLUDED_DEBONDCHECKTABLE_H_
