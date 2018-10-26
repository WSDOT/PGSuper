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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\CapacityToDemand.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsCapacityToDemand
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////
rptCapacityToDemand::rptCapacityToDemand():
rptRcString("Undef"),
m_RcSymbolInfinity(rptRcSymbol::INFINITY),
m_Capacity(0.0), m_Demand(0.0), m_Passed(false)
{
   Init();
}

rptCapacityToDemand::rptCapacityToDemand(Float64 capacity, Float64 demand, bool passed):
rptRcString("Undef"),
m_RcSymbolInfinity(rptRcSymbol::INFINITY),
m_Capacity(capacity), m_Demand(demand), m_Passed(passed)
{
   Init();

   rptRcString::SetValue(this->AsString().c_str());
}

void rptCapacityToDemand::Init()
{
   m_FormatTool.SetFormat(sysNumericFormatTool::Fixed);
   m_FormatTool.SetPrecision(2);
}


rptCapacityToDemand::rptCapacityToDemand(const rptCapacityToDemand& rOther) :
m_RcSymbolInfinity(rptRcSymbol::INFINITY),
rptRcString(rOther)
{
   MakeCopy( rOther );
}

rptCapacityToDemand& rptCapacityToDemand::operator = (const rptCapacityToDemand& rOther)
{
   if ( this != &rOther )
      MakeAssignment( rOther );

   return *this;
}

rptReportContent* rptCapacityToDemand::CreateClone() const
{
   return new rptCapacityToDemand( *this );
}

rptReportContent& rptCapacityToDemand::SetValue(Float64 capacity, Float64 demand, bool passed)
{
   m_Capacity = capacity;
   m_Demand = demand;
   m_Passed = passed;

   if (!IsEqual(m_Capacity,0.0) && IsEqual(m_Demand,0.0))
      return m_RcSymbolInfinity;
   else
      return rptRcString::SetValue(this->AsString().c_str());
}

std::string rptCapacityToDemand::AsString() const
{
   if(IsEqual(m_Capacity,0.0))
   {
      ATLASSERT(0); // c/d for c==0.0 makes no sense - return return 0.00, but this should be caught by caller
      return std::string("0.00"); 
   }
   else if (IsEqual(m_Demand,0.0))
   {
      return std::string("inf"); 
   }
   else if (Sign(m_Capacity) != Sign(m_Demand))
   {
      // Cannot have negative c/d
      return std::string("-");
   }
   else
   {
      Float64 cd = m_Capacity/m_Demand;

      if (cd > 10.0)
      {
         return std::string("10+");
      }
      else
      {
         std::string cds = m_FormatTool.AsString(cd);

         if (cds==std::string("1.00") && !m_Passed)
         {
            // Force out of tolerance value to report 0.99
            return std::string("0.99");
         }

         // Report computed value
         return cds;
      }
   }
}

void rptCapacityToDemand::MakeCopy(const rptCapacityToDemand& rOther)
{
   m_Capacity = rOther.m_Capacity;
   m_Demand = rOther.m_Demand;
   m_Passed = rOther.m_Passed;
}

void rptCapacityToDemand::MakeAssignment(const rptCapacityToDemand& rOther)
{
   MakeCopy( rOther );
}