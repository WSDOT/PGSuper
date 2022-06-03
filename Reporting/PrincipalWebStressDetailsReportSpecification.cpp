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

#include "StdAfx.h"
#include <Reporting\PrincipalWebStressDetailsReportSpecification.h>
#include <IFace\PointOfInterest.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPrincipalWebStressDetailsReportSpecification::CPrincipalWebStressDetailsReportSpecification(LPCTSTR strReportName,IBroker* pBroker,bool bReportAtAllLocations,const pgsPointOfInterest& poi,IntervalIndexType intervalIdx, bool bReportAxial, bool bReportShear) :
CBrokerReportSpecification(strReportName,pBroker)
{
   SetOptions(bReportAtAllLocations,poi,intervalIdx, bReportAxial, bReportShear);
}

CPrincipalWebStressDetailsReportSpecification::~CPrincipalWebStressDetailsReportSpecification(void)
{
}

void CPrincipalWebStressDetailsReportSpecification::SetOptions(bool bReportAtAllLocations,const pgsPointOfInterest& poi,IntervalIndexType intervalIdx, bool bReportAxial, bool bReportShear)
{
   m_bReportAtAllLocations = bReportAtAllLocations;
   m_Poi = poi;
   m_IntervalIdx = intervalIdx;
   m_bReportShearDetails = bReportShear;
   m_bReportAxialDetails = bReportAxial;
}

HRESULT CPrincipalWebStressDetailsReportSpecification::Validate() const
{
   // TODO: Validate report parameters and license
   // This function is used to validate the reporting parameters
   // I think it can also be used to validate the license. If the license isn't
   // valid, don't create the report???
   return S_OK;
}

std::_tstring CPrincipalWebStressDetailsReportSpecification::GetReportContextString() const
{
   CGirderKey girderKey = m_Poi.GetSegmentKey();
   if ( girderKey.groupIndex != ALL_SPANS && girderKey.girderIndex != ALL_GIRDERS )
   {
      CComPtr<IBroker> pBroker;
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      rptPointOfInterest rptPoi(&pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
      rptPoi.SetValue(POI_SPAN,m_Poi);
      rptPoi.PrefixAttributes(false); // put the attributes after the location
      rptPoi.IncludeSpanAndGirder(true);
      CString strLabel;
      strLabel.Format(_T("%s"), rptPoi.AsString().c_str());

      strLabel.Replace(_T("<sub>"), _T(""));
      strLabel.Replace(_T("</sub>"), _T(""));

      return std::_tstring(strLabel);
   }

   return std::_tstring();
}

bool CPrincipalWebStressDetailsReportSpecification::ReportAtAllLocations() const
{
   return m_bReportAtAllLocations;
}

bool CPrincipalWebStressDetailsReportSpecification::ReportAxial() const
{
   return m_bReportAxialDetails;
}

bool CPrincipalWebStressDetailsReportSpecification::ReportShear() const
{
   return m_bReportShearDetails;
}

pgsPointOfInterest CPrincipalWebStressDetailsReportSpecification::GetPointOfInterest() const
{
   return m_Poi;
}

IntervalIndexType CPrincipalWebStressDetailsReportSpecification::GetInterval() const
{
   return m_IntervalIdx;
}
