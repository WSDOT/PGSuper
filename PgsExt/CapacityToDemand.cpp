///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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



// Returns true if C/D 1 is less than C/D 2
bool IsCDLess(cdSense sense, Float64 capacity1, Float64 demand1, Float64 capacity2, Float64 demand2)
{
   // First do some input sanity checking
#if defined _DEBUG
   if (sense==cdPositive)
   {
      ATLASSERT(capacity1>=0.0 && capacity2>=0.0); // Signs of capacites cannot be different
   }
   else
   {
      ATLASSERT(capacity1<=0.0 && capacity2<=0.0); // ""
   }
#endif

   // We must deal with all of the edge cases
   bool icz1 = IsZero(capacity1);
   bool icz2 = IsZero(capacity2);
   if (icz1 && icz2)
   {
      // Both capacities are zero. Largest demand wins
      if (sense==cdPositive)
      {
         return demand1 > demand2;
      }
      else
      {
         return demand1 < demand2;
      }
   }
   else if (icz1)
   {
      // Capacity 1 is zero, and capacity 2 is not. Zero always wins
      return true;
   }
   else if (icz2)
   {
      // Capacity 2 is zero, and capacity 1 is not. Zero always wins
      return false;
   }
   else
   {
      // We have two non-zero capacities.
      // Make sure we don't divide by zero
      bool idz1 = IsZero(demand1);
      bool idz2 = IsZero(demand2);
      if (idz1 && idz2)
      {
         // Both demands are zero. smallest capacity wins
         if (sense==cdPositive)
         {
            return capacity1 < capacity2;
         }
         else
         {
            return capacity1 > capacity2;
         }
      }
      else if (idz1)
      {
         return false; // c/0 == inf
      }
      else if (idz2)
      {
         return true;
      }
      else
      {
         // We have numbers...
         Float64 cd1 = capacity1/demand1;
         Float64 cd2 = capacity2/demand2;

         // But, have to deal with negative c/d's
         if (cd2 < 0.0)
         {
            if(cd1>0.0)
            {
               return true;
            }
            else
            {
               // both negative
               return cd1 < cd2;
            }
         }
         else
         {
            if (cd1 < 0.0)
            {
               return false;
            }
            else
            {
               // Finally can compare two positive c/d's
               return cd1 < cd2;
            }
         }
      }
   }
}

// Function to determine tensile stress and capacity due to controlling impact
// 
void DetermineControllingTensileStress(Float64 fUp, Float64 fNone, Float64 fDown, 
                                       Float64 capUp, Float64 capNone, Float64 capDown,
                                       Float64* pfCtrl, Float64 * pCapCtrl)
{
   if (fUp<=TOLERANCE && fNone<=TOLERANCE && fDown<=TOLERANCE)
   {
      // Entire section is in compression always. Just use max stress for demand and min for capacity
      *pfCtrl = Max(fUp, fNone, fDown);
      *pCapCtrl = capUp; // which doesn't matter. since we have all compression, they are all minimum
   }
   else
   {
      // Have tension for at least one impact. Find min c/d of the three
      // Use function to compare c/d's
      if ( IsCDLess(cdPositive, capUp, fUp, capNone, fNone))
      {
         // Up is less than none, compare to down
         if ( IsCDLess(cdPositive, capUp, fUp, capDown, fDown))
         {
            *pfCtrl   = fUp;
            *pCapCtrl = capUp;
         }
      }
      else if ( IsCDLess(cdPositive, capNone, fNone, capDown, fDown))
      {
         *pfCtrl   = fNone;
         *pCapCtrl = capNone;
      }
      else
      {
         *pfCtrl   = fDown;
         *pCapCtrl = capDown;
      }
   }
}

/****************************************************************************
CLASS
   pgsCapacityToDemand
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////
rptCapacityToDemand::rptCapacityToDemand():
rptRcString(_T("Undef")),
m_RcSymbolInfinity(rptRcSymbol::infinity),
m_Capacity(0.0), m_Demand(0.0), m_Passed(false)
{
   Init();
}

rptCapacityToDemand::rptCapacityToDemand(Float64 capacity, Float64 demand, bool passed):
rptRcString(_T("Undef")),
m_RcSymbolInfinity(rptRcSymbol::infinity),
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
m_RcSymbolInfinity(rptRcSymbol::infinity),
rptRcString(rOther)
{
   MakeCopy( rOther );
}

rptCapacityToDemand& rptCapacityToDemand::operator = (const rptCapacityToDemand& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment( rOther );
   }

   return *this;
}

rptReportContent* rptCapacityToDemand::CreateClone() const
{
   return new rptCapacityToDemand( *this );
}

rptReportContent& rptCapacityToDemand::SetValue(Float64 cdr)
{
   m_CDR = cdr;
   if ( cdr == CDR_INF )
   {
      return m_RcSymbolInfinity;
   }
   else if ( cdr == CDR_NA )
   {
      return rptRcString::SetValue(_T("N/A"));
   }
   else if ( cdr == CDR_SKIP )
   {
      return rptRcString::SetValue(_T("-"));
   }
   else if ( CDR_LARGE <= cdr )
   {
      return rptRcString::SetValue(_T("10+"));
   }
   else
   {
      return rptRcString::SetValue(m_FormatTool.AsString(cdr).c_str());
   }
}

rptReportContent& rptCapacityToDemand::SetValue(Float64 capacity, Float64 demand, bool passed)
{
   m_Capacity = capacity;
   m_Demand = demand;
   m_Passed = passed;

   if (!IsEqual(m_Capacity,0.0) && IsEqual(m_Demand,0.0))
   {
      return m_RcSymbolInfinity;
   }
   else
   {
      return rptRcString::SetValue(this->AsString().c_str());
   }
}

std::_tstring rptCapacityToDemand::AsString() const
{
#pragma Reminder("REVIEW: Should this be an override of the base class GetString method???")
   if (Sign(m_Capacity) != Sign(m_Demand))
   {
      // Cannot have negative c/d
      return std::_tstring(_T("-"));
   }
   else if(IsEqual(m_Capacity,0.0))
   {
//      ATLASSERT(false); // c/d for c==0.0 makes no sense - return return 0.00, but this should be caught by caller
      return std::_tstring(_T("0.00")); 
   }
   else if (IsEqual(m_Demand,0.0))
   {
      return std::_tstring(_T("inf")); 
   }
   else
   {
      Float64 cd = m_Capacity/m_Demand;

      if (10.0 < cd)
      {
         return std::_tstring(_T("10+"));
      }
      else
      {
         std::_tstring cds = m_FormatTool.AsString(cd);

         if (cds==std::_tstring(_T("1.00")) && !m_Passed)
         {
            // Force out of tolerance value to report 0.99
            return std::_tstring(_T("0.99"));
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