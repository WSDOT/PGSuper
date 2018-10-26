///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

///////////////////////////////////////////////////////////////////////////////
// But first, an inline function from comparing two pairs of capacity/demands
///////////////////////////////////////////////////////////////////////////////
// Sense of capacity being measured. (e.g., tension positive, compression negative)
// This is necessary because both capacities being compared could be zero.
enum cdSense {cdPositive, cdNegative};

// Returns true if C/D 1 is less than C/D 2
inline bool IsCDLess(cdSense sense, Float64 capacity1, Float64 demand1, Float64 capacity2, Float64 demand2)
{
   // First do some input sanity checking
#if defined _DEBUG
   if (sense==cdPositive)
      ATLASSERT(capacity1>=0.0 && capacity2>=0.0); // Signs of capacites cannot be different
   else
      ATLASSERT(capacity1<=0.0 && capacity2<=0.0); // ""
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
inline void DetermineControllingTensileStress(Float64 fUp, Float64 fNone, Float64 fDown, 
                                              Float64 capUp, Float64 capNone, Float64 capDown,
                                              Float64* pfCtrl, Float64 * pCapCtrl)
{
   if (fUp<=TOLERANCE && fNone<=TOLERANCE && fDown<=TOLERANCE)
   {
      // Entire section is in compression always. Just use max stress for demand and min for capacity
      *pfCtrl = Max3(fUp, fNone, fDown);
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
   virtual std::_tstring AsString() const;


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

#define RF_PASS(_cd_,_rf_) _cd_.SetValue(_rf_,1.0,true)
#define RF_FAIL(_cd_,_rf_) color(Red) << _cd_.SetValue(_rf_ < 0 ? 0 : _rf_,1.0,false) << color(Black)