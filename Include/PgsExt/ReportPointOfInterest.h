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

#pragma once
#include <PgsExt\PgsExtExp.h>
#include <PgsExt\PointOfInterest.h>
#include <Reporter\Reporter.h>

/*****************************************************************************
CLASS 
   rptPointOfInterest

   Report content for points of interest.


DESCRIPTION
   Report content for points of interest. Reports the distance from start,
   adjust for an end offset, and annotates special POI.

LOG
   rab : 02.04.1999 : Created file
*****************************************************************************/
class PGSEXTCLASS rptPointOfInterest : public rptLengthUnitValue
{
public:
   //------------------------------------------------------------------------
   rptPointOfInterest(const WBFL::Units::Length* pUnitOfMeasure = 0,
                      Float64 zeroTolerance = 0.,
                      bool bShowUnitTag = true);

   //------------------------------------------------------------------------
   rptPointOfInterest(const rptPointOfInterest& rOther);

   //------------------------------------------------------------------------
   rptPointOfInterest& operator = (const rptPointOfInterest& rOther);

   //------------------------------------------------------------------------
   virtual rptReportContent* CreateClone() const override;

   //------------------------------------------------------------------------
   virtual rptReportContent& SetValue(PoiAttributeType reference,const pgsPointOfInterest& poi);

   //------------------------------------------------------------------------
   std::_tstring AsString() const;

   // Prefixes the POI with Span s Girder g
   void IncludeSpanAndGirder(bool bIncludeSpanAndGirder);
   bool IncludeSpanAndGirder() const;

   //------------------------------------------------------------------------
   // If set to true, the poi attribute prefixes the distance value
   // eg  (HP) 2.34ft, otherwise it post-fixes the distance value 2.34ft (HP)
   void PrefixAttributes(bool bPrefixAttributes=true);

   //------------------------------------------------------------------------
   // Returns true if the poi is to be prefixed
   bool PrefixAttributes() const;

protected:
   void MakeCopy(const rptPointOfInterest& rOther);
   void MakeAssignment(const rptPointOfInterest& rOther);

private:
   CComPtr<IBroker> m_pBroker;
   pgsPointOfInterest m_POI;
   CSpanKey m_SpanKey;
   Float64 m_Xspan;
   Float64 m_Xgl;
   PoiAttributeType m_Reference;
   bool m_bPrefixAttributes;
   bool m_bIncludeSpanAndGirder;
};
