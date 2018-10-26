///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include <Reporter\Reporter.h>

/*****************************************************************************
CLASS 
   rptCapacityToDemand

   Report content for Capacity/Demand ratios


DESCRIPTION
   Report content for Capacity/Demand ratios. A consistant format for C/D Reporting


COPYRIGHT
   Copyright © 1997-2009
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 12.18.2009 : Created file
*****************************************************************************/
class PGSEXTCLASS rptCapacityToDemand : public rptRcString
{
public:
   //------------------------------------------------------------------------
   rptCapacityToDemand();

   //------------------------------------------------------------------------
   rptCapacityToDemand(Float64 capacity, Float64 demand, bool passed);

   //------------------------------------------------------------------------
   rptCapacityToDemand(const rptCapacityToDemand& rOther);

   //------------------------------------------------------------------------
   rptCapacityToDemand& operator = (const rptCapacityToDemand& rOther);

   //------------------------------------------------------------------------
   virtual rptReportContent* CreateClone() const;

   //------------------------------------------------------------------------
   virtual rptReportContent& SetValue(Float64 capacity, Float64 demand, bool passed);

   //------------------------------------------------------------------------
   virtual std::string AsString() const;


protected:
   void MakeCopy(const rptCapacityToDemand& rOther);
   void MakeAssignment(const rptCapacityToDemand& rOther);

private:
   void Init();

   Float64 m_Capacity;
   Float64 m_Demand;
   bool    m_Passed;

   sysNumericFormatTool m_FormatTool;
   rptRcSymbol m_RcSymbolInfinity;
};
