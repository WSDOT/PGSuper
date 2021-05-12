///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

interface IBridge;
interface IEAFDisplayUnits;
class pgsHorizontalShearArtifact;

/*****************************************************************************
CLASS 
   CInterfaceShearDetails

   Encapsulates the construction of the horizontal interface shear check table.


DESCRIPTION
   Encapsulates the construction of the horizontal interface shear check table.

LOG
   rdp : 12.26.1998 : Created file
*****************************************************************************/

class REPORTINGCLASS CInterfaceShearDetails
{
public:
   //------------------------------------------------------------------------
   // Default constructor
   CInterfaceShearDetails(IEAFDisplayUnits* pDisplayUnits);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CInterfaceShearDetails();

   //------------------------------------------------------------------------
   // Builds the table.
   void Build(IBroker* pBroker, rptChapter* pChapter, const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits, IntervalIndexType intervalIdx, pgsTypes::LimitState ls);

protected:
   void BuildDesign(IBroker* pBroker, rptChapter* pChapter, const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits, IntervalIndexType intervalIdx, pgsTypes::LimitState ls);
   void BuildRating(IBroker* pBroker, rptChapter* pChapter, const CGirderKey& girderKey, IEAFDisplayUnits* pDisplayUnits, IntervalIndexType intervalIdx, pgsTypes::LimitState ls);

private:
   //------------------------------------------------------------------------
   // Copy constructor
   CInterfaceShearDetails(const CInterfaceShearDetails& rOther) = delete;

   DECLARE_UV_PROTOTYPE(rptPointOfInterest, location);
   DECLARE_UV_PROTOTYPE(rptForceUnitValue, shear);
   DECLARE_UV_PROTOTYPE(rptForcePerLengthUnitValue, shear_per_length);
   DECLARE_UV_PROTOTYPE(rptStressUnitValue, fy);
   DECLARE_UV_PROTOTYPE(rptStressUnitValue, stress);
   DECLARE_UV_PROTOTYPE(rptStressUnitValue, stress_with_tag);
   DECLARE_UV_PROTOTYPE(rptAreaPerLengthValue, AvS);
   DECLARE_UV_PROTOTYPE(rptLengthUnitValue, dim);
   DECLARE_UV_PROTOTYPE(rptAreaUnitValue, area);
   DECLARE_UV_PROTOTYPE(rptLength3UnitValue, l3);
   DECLARE_UV_PROTOTYPE(rptLength4UnitValue, l4);

   bool m_bIsSpec2007orOlder;
   ShearFlowMethod m_ShearFlowMethod;

   rptRcTable* CreateVuiTable(IBroker* pBroker, rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   void FillVuiTable(rptRcTable* pTable, RowIndexType row, const pgsPointOfInterest& poi, const pgsHorizontalShearArtifact* pArtifact);
   rptRcTable* CreateAvfTable(IEAFDisplayUnits* pDisplayUnits);
   void FillAvfTable(rptRcTable* pTable, RowIndexType row, const pgsPointOfInterest& poi, const pgsHorizontalShearArtifact* pArtifact);
   rptRcTable* CreateVniTable(IBroker* pBroker, rptChapter* pChapter, IEAFDisplayUnits* pDisplayUnits, const std::vector<std::pair<SegmentIndexType, const pgsHorizontalShearArtifact*>>& vSegmentArtifacts, std::vector<std::pair<SegmentIndexType, const pgsHorizontalShearArtifact*>>& vClosureArtifacts);
   void FillVniTable(rptRcTable* pTable, RowIndexType row, const pgsPointOfInterest& poi, const pgsHorizontalShearArtifact* pArtifact);
   rptRcTable* CreateMinAvfTable(rptChapter* pChapter, IBridge* pBridge, IEAFDisplayUnits* pDisplayUnits, bool bIsRoughened, bool doAllStirrupsEngageDeck);
   void FillMinAvfTable(rptRcTable* pTable, RowIndexType row, const pgsPointOfInterest& poi, const pgsHorizontalShearArtifact* pArtifact, Float64 llss, IEAFDisplayUnits* pDisplayUnits);
};
