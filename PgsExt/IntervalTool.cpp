///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <PgsExt\IntervalTool.h>
#include <MathEx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IntervalTool::IntervalTool(const unitmgtScalar& umd) : 
ScalarTool(umd)
{
   m_LastValue = -1;
}

void IntervalTool::SetLastValue(Float64 value)
{
   m_LastValue = value;
}

std::_tstring IntervalTool::AsString(Float64 value) const
{
   ATLASSERT(0.0 <= m_LastValue);

   // Intervals are displayed as int's
   int v = (int)value;
   if ( value == (int)m_LastValue )
   {
      v -= 1;
   }


   std::_tostringstream os;
   os << v;

   std::_tstring strValue(os.str());

   if ( value == m_LastValue )
   {
      strValue += _T("e");
   }
   else if ( value - (int)value == 0.0 )
   {
      strValue += _T("b");
   }
   else if ( value - (int)value == 0.5 )
   {
      strValue += _T("m");
   }

   return strValue;
}
