///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <Stability\AnalysisPoint.h>
#include <PgsExt\PointOfInterest.h>

class PGSEXTCLASS pgsStabilityAnalysisPoint : public stbIAnalysisPoint
{
public:
   pgsStabilityAnalysisPoint();
   pgsStabilityAnalysisPoint(const pgsPointOfInterest& poi,PoiAttributeType refAttrib);
   pgsStabilityAnalysisPoint(const pgsStabilityAnalysisPoint& other);
   virtual ~pgsStabilityAnalysisPoint();

   void SetPointOfInterest(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPointOfInterest() const;

   void SetReferenceAttribute(PoiAttributeType refAttrib);
   PoiAttributeType GetReferenceAttribute() const;

   virtual Float64 GetLocation() const;

   virtual std::_tstring  AsString(const unitmgtLengthData& lengthUnit,Float64 offset,bool bShowUnit) const;

   virtual stbIAnalysisPoint* Clone() const;

protected:
   pgsPointOfInterest m_Poi;
   PoiAttributeType m_RefAttribute;
};