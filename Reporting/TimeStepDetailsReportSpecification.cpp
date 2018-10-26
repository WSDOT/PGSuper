///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <Reporting\TimeStepDetailsReportSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimeStepDetailsReportSpecification::CTimeStepDetailsReportSpecification(LPCTSTR strReportName,IBroker* pBroker,bool bReportAtAllLocations,const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) :
CBrokerReportSpecification(strReportName,pBroker)
{
   m_bReportAtAllLocations = bReportAtAllLocations;
   m_Poi = poi;
   m_IntervalIdx = intervalIdx;
}

CTimeStepDetailsReportSpecification::~CTimeStepDetailsReportSpecification(void)
{
}

HRESULT CTimeStepDetailsReportSpecification::Validate() const
{
   // TODO: Validate report parameters and license
   // This function is used to validate the reporting parameters
   // I think it can also be used to validate the license. If the license isn't
   // valid, don't create the report???
   return S_OK;
}

bool CTimeStepDetailsReportSpecification::ReportAtAllLocations() const
{
   return m_bReportAtAllLocations;
}

pgsPointOfInterest CTimeStepDetailsReportSpecification::GetPointOfInterest() const
{
   return m_Poi;
}

IntervalIndexType CTimeStepDetailsReportSpecification::GetInterval() const
{
   return m_IntervalIdx;
}
