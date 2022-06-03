///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

/// Abstract representation of a prestress development length
class PGSEXTCLASS pgsDevelopmentLength
{
public:
   pgsDevelopmentLength::pgsDevelopmentLength() :
      m_fpe(0.0), m_fps(0.0)
   {
   }

   pgsDevelopmentLength::pgsDevelopmentLength(Float64 fpe, Float64 fps) :
      m_fpe(fpe), m_fps(fps)
   {
   }


   void SetFpe(Float64 fpe)
   {
      m_fpe = fpe;
   }

   Float64 GetFpe() const
   {
      return m_fpe;
   }

   void SetFps(Float64 fps)
   {
      m_fps = fps;
   }

   Float64 GetFps() const
   {
      return m_fps;
   }


   virtual Float64 GetDevelopmentLength() const = 0;

protected:
   Float64 m_fpe;
   Float64 m_fps;
};
