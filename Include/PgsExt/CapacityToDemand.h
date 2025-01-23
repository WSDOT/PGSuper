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

#pragma once
#include <PgsExt\PgsExtExp.h>
#include <Reporter\Reporter.h>

#define CDR_NA   -1  // reports "N/A"
#define CDR_SKIP -2  // reports "-"
#define CDR_INF  9999999 // reports infinity symbol
#define CDR_LARGE 10 // anything larger than this value is reported as 10+

/*****************************************************************************
CLASS 
   rptCapacityToDemand

   Report content for Capacity/Demand ratios


DESCRIPTION
   Report content for Capacity/Demand ratios. A consistant format for C/D Reporting

LOG
   rdp : 12.18.2009 : Created file
*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Functions for comparing two pairs of capacity/demands
///////////////////////////////////////////////////////////////////////////////
// Sense of capacity being measured. (e.g., tension positive, compression negative)
// This is necessary because both capacities being compared could be zero.
enum cdSense {cdPositive, cdNegative};

// Returns true if C/D 1 is less than C/D 2
bool PGSEXTFUNC IsCDLess(cdSense sense, Float64 capacity1, Float64 demand1, Float64 capacity2, Float64 demand2);

// Function to determine tensile stress and capacity due to controlling impact
void PGSEXTFUNC DetermineControllingTensileStress(Float64 fUp, Float64 fNone, Float64 fDown, 
                                                  Float64 capUp, Float64 capNone, Float64 capDown,
                                                  Float64* pfCtrl, Float64 * pCapCtrl);

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
   virtual rptReportContent* CreateClone() const override;

   //------------------------------------------------------------------------
   virtual rptReportContent& SetValue(Float64 cdr);
   virtual rptReportContent& SetValue(Float64 capacity, Float64 demand, bool passed);

   //------------------------------------------------------------------------
   virtual std::_tstring AsString() const;


protected:
   void MakeCopy(const rptCapacityToDemand& rOther);
   void MakeAssignment(const rptCapacityToDemand& rOther);

private:
   void Init();

   Float64 m_Capacity;
   Float64 m_Demand;
   bool    m_Passed;

   Float64 m_CDR;

   WBFL::System::NumericFormatTool m_FormatTool;
   rptRcSymbol m_RcSymbolInfinity;
};

#define RF_PASS(_cd_,_rf_) _cd_.SetValue(_rf_,1.0,true)
#define RF_FAIL(_cd_,_rf_) color(Red) << _cd_.SetValue(_rf_ < 0 ? 0 : _rf_,0 < _rf_ ? 1 : 0,false) << color(Black)