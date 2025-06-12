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

#include <Reporting\ReportingExp.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>

#include "ReportNotes.h"

class IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CUserMomentsTable

   Encapsulates the construction of the User forces table.


DESCRIPTION
   Encapsulates the construction of the User forces table.

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CUserMomentsTable
{
public:
   CUserMomentsTable();

   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx,
                             std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const;
};

template <class M,class T>
rptRcTable* CreateUserLoadHeading(LPCTSTR strTitle,bool bPierTable,pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits,const T& unitT)
{
   ColumnIndexType nCols = 4;
   if ( analysisType == pgsTypes::Envelope )
   {
      nCols += 3;
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,strTitle);

   ColumnIndexType col = 0;
   
   // Set up table headings
   if ( bPierTable )
   {
      (*pTable)(0,col) << _T("");
   }
   else
   {
      (*pTable)(0, col) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
   }

   if ( analysisType == pgsTypes::Envelope )
   {
      pTable->SetNumberOfHeaderRows(2);
      pTable->SetRowSpan(0, col++,2);

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0, col) << _T("User DC");
      (*pTable)(1, col++) << COLHDR(_T("Max"), M, unitT);
      (*pTable)(1, col++) << COLHDR(_T("Min"), M, unitT);

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0,col) << _T("User DW");
      (*pTable)(1, col++) << COLHDR(_T("Max"), M, unitT);
      (*pTable)(1, col++) << COLHDR(_T("Min"), M, unitT);

      pTable->SetColumnSpan(0,col,2);
      (*pTable)(0,col) << _T("User LL+IM");
      (*pTable)(1, col++) << COLHDR(_T("Max"), M, unitT);
      (*pTable)(1, col++) << COLHDR(_T("Min"), M, unitT);
   }
   else
   {
      col++;
      pTable->SetNumberOfHeaderRows(1);
      (*pTable)(0, col++) << COLHDR(_T("User DC"),          M, unitT );
      (*pTable)(0, col++) << COLHDR(_T("User DW"),          M, unitT );
      (*pTable)(0, col++) << COLHDR(_T("User LL+IM"),       M, unitT );
   }

   return pTable;
}
