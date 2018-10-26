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
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>

#include <PgsExt\Keys.h>

class pgsGirderArtifact;

/*****************************************************************************
CLASS 
   CSpecCheckSummaryChapterBuilder

   Writes out a summary of the spec chec.


DESCRIPTION
   Writes out a summary of the spec chec. The summary is either PASS or FAILED
   with a listing of everything that failed.

COPYRIGHT
   Copyright © 1997-1999
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 03.17.1999 : Created file
*****************************************************************************/

class REPORTINGCLASS CSpecCheckSummaryChapterBuilder : public CPGSuperChapterBuilder
{
public:
   CSpecCheckSummaryChapterBuilder(bool referToDetailsReport,bool bSelect = true);

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const;

   //------------------------------------------------------------------------
   virtual rptChapter* BuildEx(CReportSpecification* pRptSpec,Uint16 level,
                               const pgsGirderArtifact* pGirderArtifact) const;

   //------------------------------------------------------------------------
   void CreateContent(rptChapter* pChapter, IBroker* pBroker,
                      const pgsGirderArtifact* pGirderArtifact) const;


   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const;

protected:

private:
   bool m_ReferToDetailsReport;

   // Prevent accidental copying and assignment
   CSpecCheckSummaryChapterBuilder(const CSpecCheckSummaryChapterBuilder&);
   CSpecCheckSummaryChapterBuilder& operator=(const CSpecCheckSummaryChapterBuilder&);
};
