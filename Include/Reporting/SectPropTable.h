///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#ifndef INCLUDED_SECTPROPTABLE_H_
#define INCLUDED_SECTPROPTABLE_H_

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CSectionPropertiesTable

   Encapsulates the construction of the section properties table.


DESCRIPTION
   Encapsulates the construction of the section properties table.

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

/// <summary>
/// Tabular list of gross section properties for prismatic section (lists a single set of properties)
/// </summary>
class REPORTINGCLASS CSectionPropertiesTable
{
public:
   CSectionPropertiesTable() = default;
   CSectionPropertiesTable(const CSectionPropertiesTable& rOther) = default;
   ~CSectionPropertiesTable() = default;
   CSectionPropertiesTable& operator=(const CSectionPropertiesTable& rOther) = default;

   /// <summary>
   /// 
   /// </summary>
   /// <param name="pBroker"></param>
   /// <param name="segmentKey"></param>
   /// <param name="bComposite">If true, lists properties for both the composite and non-composite section, otherwise just the non-composite section properties are listed</param>
   /// <param name="pDisplayUnits"></param>
   /// <returns></returns>
   rptRcTable* Build(IBroker* pBroker,const CSegmentKey& segmentKey,bool bComposite, IEAFDisplayUnits* pDisplayUnits) const;
};

#endif // INCLUDED_SECTPROPTABLE_H_
