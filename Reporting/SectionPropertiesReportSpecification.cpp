///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include "stdafx.h"
#include <Reporting\SectionPropertiesReportSpecification.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <EAF\EAFDisplayUnits.h>
#include <AgentTools.h>


CSectionPropertiesReportSpecification::CSectionPropertiesReportSpecification(const std::_tstring& strReportName, 
    std::weak_ptr<WBFL::EAF::Broker> pBroker, const pgsPointOfInterest& poi, IntervalIndexType intervalIdx) :
    CPoiReportSpecification(strReportName, pBroker, poi)
{
    m_interval = intervalIdx;
}

CSectionPropertiesReportSpecification::~CSectionPropertiesReportSpecification(void)
{
}

void CSectionPropertiesReportSpecification::SetOptions(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx)
{
    SetPOI(poi);
    m_interval = intervalIdx;
}

IntervalIndexType CSectionPropertiesReportSpecification::GetInterval() const
{
    return m_interval;
}

std::_tstring CSectionPropertiesReportSpecification::GetReportContextString() const
{
    GET_IFACE2(GetBroker(), IIntervals, pIntervals);
    GET_IFACE2(GetBroker(), IEAFDisplayUnits, pDisplayUnits);

    const CSegmentKey& segmentKey = m_Poi.GetSegmentKey();

    rptPointOfInterest rptPoi(&pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
    rptPoi.SetValue(POI_SPAN, m_Poi);
    rptPoi.PrefixAttributes(false); // put the attributes after the location
    rptPoi.IncludeSpanAndGirder(true);

    CString msg;
    msg.Format(_T("%s, Interval %d: %s"), rptPoi.AsString().c_str(), 
        m_interval + 1, pIntervals->GetDescription(m_interval).c_str());
    msg.Replace(_T("<sub>"), _T(""));
    msg.Replace(_T("</sub>"), _T(""));
    return std::_tstring(msg);

}


