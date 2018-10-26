///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <PgsExt\LoadFactors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLoadFactors
****************************************************************************/


CLoadFactors::CLoadFactors()
{
   DCmin[pgsTypes::ServiceI]   = 1.0;       DCmax[pgsTypes::ServiceI]   = 1.0;
   DWmin[pgsTypes::ServiceI]   = 1.0;       DWmax[pgsTypes::ServiceI]   = 1.0;
   LLIMmin[pgsTypes::ServiceI] = 1.0;       LLIMmax[pgsTypes::ServiceI] = 1.0;

   DCmin[pgsTypes::ServiceIA]   = 0.5;      DCmax[pgsTypes::ServiceIA]   = 0.5;
   DWmin[pgsTypes::ServiceIA]   = 0.5;      DWmax[pgsTypes::ServiceIA]   = 0.5;
   LLIMmin[pgsTypes::ServiceIA] = 1.0;      LLIMmax[pgsTypes::ServiceIA] = 1.0;

   DCmin[pgsTypes::ServiceIII]   = 1.0;     DCmax[pgsTypes::ServiceIII]   = 1.0;
   DWmin[pgsTypes::ServiceIII]   = 1.0;     DWmax[pgsTypes::ServiceIII]   = 1.0;
   LLIMmin[pgsTypes::ServiceIII] = 0.8;     LLIMmax[pgsTypes::ServiceIII] = 0.8;

   DCmin[pgsTypes::StrengthI]   = 0.90;     DCmax[pgsTypes::StrengthI]   = 1.25;
   DWmin[pgsTypes::StrengthI]   = 0.65;     DWmax[pgsTypes::StrengthI]   = 1.50;
   LLIMmin[pgsTypes::StrengthI] = 1.75;     LLIMmax[pgsTypes::StrengthI] = 1.75;

   DCmin[pgsTypes::StrengthII]   = 0.90;    DCmax[pgsTypes::StrengthII]   = 1.25;
   DWmin[pgsTypes::StrengthII]   = 0.65;    DWmax[pgsTypes::StrengthII]   = 1.50;
   LLIMmin[pgsTypes::StrengthII] = 1.35;    LLIMmax[pgsTypes::StrengthII] = 1.35;

   DCmin[pgsTypes::FatigueI]   = 0.5;      DCmax[pgsTypes::FatigueI]   = 0.5;
   DWmin[pgsTypes::FatigueI]   = 0.5;      DWmax[pgsTypes::FatigueI]   = 0.5;
   LLIMmin[pgsTypes::FatigueI] = 1.0;      LLIMmax[pgsTypes::FatigueI] = 1.0;
}

CLoadFactors::CLoadFactors(const CLoadFactors& rOther)
{
   MakeCopy(rOther);
}

CLoadFactors& CLoadFactors::operator=(const CLoadFactors& rOther)
{
   MakeAssignment(rOther);
   return *this;
}

bool CLoadFactors::operator==(const CLoadFactors& rOther) const
{
   for ( int i = 0; i < 6; i++ )
   {
      if ( DCmin[i] != rOther.DCmin[i] )
         return false;

      if ( DWmin[i] != rOther.DWmin[i] )
         return false;

      if ( LLIMmin[i] != rOther.LLIMmin[i] )
         return false;

      if ( DCmax[i] != rOther.DCmax[i] )
         return false;

      if ( DWmax[i] != rOther.DWmax[i] )
         return false;

      if ( LLIMmax[i] != rOther.LLIMmax[i] )
         return false;
   }

   return true;
}

bool CLoadFactors::operator!=(const CLoadFactors& rOther) const
{
   return !CLoadFactors::operator==(rOther);
}

void CLoadFactors::MakeCopy(const CLoadFactors& rOther)
{
   for ( int i = 0; i < 6; i++ )
   {
      DCmin[i] = rOther.DCmin[i];
      DWmin[i] = rOther.DWmin[i];
      LLIMmin[i] = rOther.LLIMmin[i];
      DCmax[i] = rOther.DCmax[i];
      DWmax[i] = rOther.DWmax[i];
      LLIMmax[i] = rOther.LLIMmax[i];
   }
}

void CLoadFactors::MakeAssignment(const CLoadFactors& rOther)
{
   MakeCopy(rOther);
}
