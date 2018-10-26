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

#ifndef INCLUDED_USERMOMENTSTABLE_H_
#define INCLUDED_USERMOMENTSTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>
#include "ReportNotes.h"

interface IDisplayUnits;

/*****************************************************************************
CLASS 
   CUserMomentsTable

   Encapsulates the construction of the User forces table.


DESCRIPTION
   Encapsulates the construction of the User forces table.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CUserMomentsTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CUserMomentsTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CUserMomentsTable(const CUserMomentsTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CUserMomentsTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CUserMomentsTable& operator = (const CUserMomentsTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::AnalysisType analysisType,
                             IDisplayUnits* pDispUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CUserMomentsTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CUserMomentsTable& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(dbgLog& rlog);
   #endif // _UNITTEST
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

template <class M,class T>
rptRcTable* CreateUserLoadHeading(const char* strTitle,bool bPierTable,pgsTypes::AnalysisType analysisType,IDisplayUnits* pDispUnits,const T& unitT)
{
   ColumnIndexType nCols = 6;
   if ( analysisType == pgsTypes::Envelope )
      nCols += 5;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,strTitle);

   // Set up table headings
   if ( !bPierTable )
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION ,    rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );
   else
      (*pTable)(0,0) << "";

   if ( analysisType == pgsTypes::Envelope )
   {
      pTable->SetNumberOfHeaderRows(3);
      pTable->SetRowSpan(0,0,3);
      pTable->SetRowSpan(1,0,-1);
      pTable->SetRowSpan(2,0,-1);

      pTable->SetColumnSpan(0,1,4);
      (*pTable)(0,1) << "Bridge Site 1";

      pTable->SetColumnSpan(0,2,4);
      (*pTable)(0,2) << "Bridge Site 2";

      pTable->SetColumnSpan(0,3,2);
      (*pTable)(0,3) << "";

      ColumnIndexType i;
      for ( i = 4; i < nCols; i++ )
         pTable->SetColumnSpan(0,i,-1);

      pTable->SetColumnSpan(1,1,2);
      (*pTable)(1,1) << "User DC";

      pTable->SetColumnSpan(1,2,2);
      (*pTable)(1,2) << "User DW";

      pTable->SetColumnSpan(1,3,2);
      (*pTable)(1,3) << "User DC";

      pTable->SetColumnSpan(1,4,2);
      (*pTable)(1,4) << "User DW";

      pTable->SetColumnSpan(1,5,2);
      (*pTable)(1,5) << "User LL+IM";

      for ( i = 6; i < nCols; i++ )
         pTable->SetColumnSpan(1,i,-1);

      (*pTable)(2,1) << COLHDR("Max", M, unitT );
      (*pTable)(2,2) << COLHDR("Min", M, unitT );
      (*pTable)(2,3) << COLHDR("Max", M, unitT );
      (*pTable)(2,4) << COLHDR("Min", M, unitT );
      (*pTable)(2,5) << COLHDR("Max", M, unitT );
      (*pTable)(2,6) << COLHDR("Min", M, unitT );
      (*pTable)(2,7) << COLHDR("Max", M, unitT );
      (*pTable)(2,8) << COLHDR("Min", M, unitT );
      (*pTable)(2,9) << COLHDR("Max", M, unitT );
      (*pTable)(2,10)<< COLHDR("Min", M, unitT );
   }
   else
   {
      pTable->SetNumberOfHeaderRows(2);
      pTable->SetRowSpan(0,0,3);
      pTable->SetRowSpan(1,0,-1);

      pTable->SetColumnSpan(0,1,2);
      (*pTable)(0,1) << "Bridge Site 1";

      pTable->SetColumnSpan(0,2,2);
      (*pTable)(0,2) << "Bridge Site 2";

      pTable->SetColumnSpan(0,3,1);
      (*pTable)(0,3) << "";

      for ( ColumnIndexType i = 4; i < nCols; i++ )
         pTable->SetColumnSpan(0,i,-1);

      (*pTable)(1,1) << COLHDR("User DC",          M, unitT );
      (*pTable)(1,2) << COLHDR("User DW",          M, unitT );
      (*pTable)(1,3) << COLHDR("User DC",          M, unitT );
      (*pTable)(1,4) << COLHDR("User DW",          M, unitT );
      (*pTable)(1,5) << COLHDR("User LL+IM",       M, unitT );
   }

   return pTable;
}

#endif // INCLUDED_UserMOMENTSTABLE_H_
