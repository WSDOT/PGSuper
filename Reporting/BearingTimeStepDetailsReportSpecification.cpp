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

#include "StdAfx.h"
#include <Reporting\BearingTimeStepDetailsReportSpecification.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBearingTimeStepDetailsReportSpecification::CBearingTimeStepDetailsReportSpecification(const std::_tstring& strReportName,IBroker* pBroker,
    bool bReportAtAllLocations,const ReactionLocation& location,IntervalIndexType intervalIdx) :
CBrokerReportSpecification(strReportName,pBroker)
{
   SetOptions(bReportAtAllLocations,location,intervalIdx);
}

CBearingTimeStepDetailsReportSpecification::~CBearingTimeStepDetailsReportSpecification(void)
{
}

void CBearingTimeStepDetailsReportSpecification::SetOptions(bool bReportAtAllLocations,const ReactionLocation& location,IntervalIndexType intervalIdx)
{
   m_bReportAtAllLocations = bReportAtAllLocations;
   m_reactionLocation = location;
   m_IntervalIdx = intervalIdx;
}

HRESULT CBearingTimeStepDetailsReportSpecification::Validate() const
{
   // TODO: Validate report parameters and license
   // This function is used to validate the reporting parameters
   // I think it can also be used to validate the license. If the license isn't
   // valid, don't create the report???
   return S_OK;
}

std::_tstring CBearingTimeStepDetailsReportSpecification::GetReportContextString() const
{
   CGirderKey girderKey = m_reactionLocation.GirderKey;
   std::_tstring strLocation;
   if (m_bReportAtAllLocations)
   {
      strLocation = _T("All Bearing Locations");
   }
   else if ( girderKey.groupIndex != ALL_GROUPS && girderKey.girderIndex != ALL_GIRDERS )
   {
      CString strLabel{ m_reactionLocation.PierLabel.c_str() };

      strLocation = std::_tstring(strLabel);

   }

   std::_tstring strIntervals;
   if (m_IntervalIdx == INVALID_INDEX)
   {
      strIntervals = _T("All Intervals");
   }
   else
   {
      std::_tostringstream os;
      os << _T("Interval ") << LABEL_INTERVAL(m_IntervalIdx);
      strIntervals = os.str();
   }

   strLocation += _T(", ") + strIntervals;
   return strLocation;
}

bool CBearingTimeStepDetailsReportSpecification::ReportAtAllLocations() const
{
   return m_bReportAtAllLocations;
}

ReactionLocation CBearingTimeStepDetailsReportSpecification::GetReactionLocation() const
{
   return m_reactionLocation;
}

IntervalIndexType CBearingTimeStepDetailsReportSpecification::GetInterval() const
{
   return m_IntervalIdx;
}
