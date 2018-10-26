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

#ifndef INCLUDED_VEHICULARLOADRESULTSTABLE_H_
#define INCLUDED_VEHICULARLOADRESULTSTABLE_H_

#include <Reporting\ReportingExp.h>
#include <IFace\AnalysisResults.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CVehicularLoadResultsTable


DESCRIPTION
   Encapsulates the construction of a table that lists moments for a given
   user defined vehicular load


COPYRIGHT
   Copyright © 1997-2007
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.19.2007 : Created file
*****************************************************************************/

class REPORTINGCLASS CVehicularLoadResultsTable
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CVehicularLoadResultsTable();

   //------------------------------------------------------------------------
   // Copy constructor
   CVehicularLoadResultsTable(const CVehicularLoadResultsTable& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CVehicularLoadResultsTable();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CVehicularLoadResultsTable& operator = (const CVehicularLoadResultsTable& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Builds the strand eccentricity table.
   virtual rptRcTable* Build(IBroker* pBroker,SpanIndexType span,GirderIndexType girder,pgsTypes::LiveLoadType llType,
                             const std::_tstring& strLLName,VehicleIndexType vehicleIndex,
                             pgsTypes::AnalysisType analysisType,
                             bool bReportTruckConfig,
                             IEAFDisplayUnits* pDisplayUnits) const;
   // GROUP: ACCESS
   // GROUP: INQUIRY
   static void ReportTruckConfiguration(const AxleConfiguration& config,rptRcTable* pTable,RowIndexType row,ColumnIndexType col,IEAFDisplayUnits* pDisplayUnits);

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS

   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CVehicularLoadResultsTable& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CVehicularLoadResultsTable& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_VEHICULARLOADRESULTSTABLE_H_
