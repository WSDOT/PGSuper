///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// The moment Capacity Details report was contributed by BridgeSight Inc.

#include "StdAfx.h"
#include <Reporting\CrackedSectionReportSpecification.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCrackedSectionReportSpecification::CCrackedSectionReportSpecification(LPCTSTR strReportName,IBroker* pBroker,const pgsPointOfInterest& poi,bool bPositiveMoment) :
   CPoiReportSpecification(strReportName,pBroker,poi)
{
   m_bPositiveMoment = bPositiveMoment;
}

CCrackedSectionReportSpecification::~CCrackedSectionReportSpecification(void)
{
}

void CCrackedSectionReportSpecification::SetOptions(const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   SetPOI(poi);
   m_bPositiveMoment = bPositiveMoment;
}

bool CCrackedSectionReportSpecification::IsPositiveMoment() const
{
   return m_bPositiveMoment;
}
