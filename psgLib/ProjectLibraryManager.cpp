///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <PsgLib\ProjectLibraryManager.h>

psgProjectLibraryManager::psgProjectLibraryManager()
{
   m_DCmin[pgsTypes::ServiceI]   = 1.0;         m_DCmax[pgsTypes::ServiceI]   = 1.0;
   m_DWmin[pgsTypes::ServiceI]   = 1.0;         m_DWmax[pgsTypes::ServiceI]   = 1.0;
   m_LLIMmin[pgsTypes::ServiceI] = 1.0;         m_LLIMmax[pgsTypes::ServiceI] = 1.0;

   m_DCmin[pgsTypes::ServiceIA]   = 0.5;        m_DCmax[pgsTypes::ServiceIA]   = 0.5;
   m_DWmin[pgsTypes::ServiceIA]   = 0.5;        m_DWmax[pgsTypes::ServiceIA]   = 0.5;
   m_LLIMmin[pgsTypes::ServiceIA] = 1.0;        m_LLIMmax[pgsTypes::ServiceIA] = 1.0;

   m_DCmin[pgsTypes::ServiceIII]   = 1.0;       m_DCmax[pgsTypes::ServiceIII]   = 1.0;
   m_DWmin[pgsTypes::ServiceIII]   = 1.0;       m_DWmax[pgsTypes::ServiceIII]   = 1.0;
   m_LLIMmin[pgsTypes::ServiceIII] = 0.8;       m_LLIMmax[pgsTypes::ServiceIII] = 0.8;

   m_DCmin[pgsTypes::StrengthI]   = 0.90;       m_DCmax[pgsTypes::StrengthI]   = 1.25;
   m_DWmin[pgsTypes::StrengthI]   = 0.65;       m_DWmax[pgsTypes::StrengthI]   = 1.50;
   m_LLIMmin[pgsTypes::StrengthI] = 1.75;       m_LLIMmax[pgsTypes::StrengthI] = 1.75;

   m_DCmin[pgsTypes::StrengthII]   = 0.90;      m_DCmax[pgsTypes::StrengthII]   = 1.25;
   m_DWmin[pgsTypes::StrengthII]   = 0.65;      m_DWmax[pgsTypes::StrengthII]   = 1.50;
   m_LLIMmin[pgsTypes::StrengthII] = 1.35;      m_LLIMmax[pgsTypes::StrengthII] = 1.35;

   m_DCmin[pgsTypes::FatigueI]   = 0.5;        m_DCmax[pgsTypes::FatigueI]   = 0.5;
   m_DWmin[pgsTypes::FatigueI]   = 0.5;        m_DWmax[pgsTypes::FatigueI]   = 0.5;
   m_LLIMmin[pgsTypes::FatigueI] = 1.5;        m_LLIMmax[pgsTypes::FatigueI] = 1.5;
}
