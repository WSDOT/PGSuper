///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\StageCompare.h>

int GetStageValue(pgsTypes::Stage stage)
{
   int value = -1;
   switch(stage)
   {
   case pgsTypes::CastingYard:               value = 1; break;
   case pgsTypes::Lifting:                   value = 2; break;
   case pgsTypes::Hauling:                   value = 3; break;
   case pgsTypes::GirderPlacement:           value = 4; break;
   case pgsTypes::TemporaryStrandRemoval:    value = 5; break;
   case pgsTypes::BridgeSite1:               value = 6; break;
   case pgsTypes::BridgeSite2:               value = 7; break;
   case pgsTypes::BridgeSite3:               value = 8; break;
   default:
      ATLASSERT(false); // is there a new stage???
   }

   return value;
}

int StageCompare(pgsTypes::Stage stage1,pgsTypes::Stage stage2)
{
   int v1 = GetStageValue(stage1);
   int v2 = GetStageValue(stage2);

   if ( v1 == v2 )
      return 0;

   if ( v1 < v2 )
      return -1;

   return 1;
}


