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

/*****************************************************************************
CLASS 
   pgsConfinementCheckArtifact

   Artifact that holds shear design/check results in the burssting zone.


DESCRIPTION
   Artifact that holds shear design/check results in the Splitting zone.

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsConfinementCheckArtifact
{
public:
   pgsConfinementCheckArtifact();
   ~pgsConfinementCheckArtifact();

   // if not applicable, the rest of the values are undefined
   bool IsApplicable() const;
   void SetApplicability(bool isAp);

   const WBFL::Materials::Rebar* GetMinBar() const;
   void SetMinBar(const WBFL::Materials::Rebar* pBar);
   Float64 GetSMax() const;
   void SetSMax(Float64 smax);

   // Factor multiplied by d (1.5)
   Float64 GetZoneLengthFactor() const;
   void SetZoneLengthFactor(Float64 fac);

   Float64 GetStartProvidedZoneLength() const;
   void SetStartProvidedZoneLength(Float64 zl);
   Float64 GetStartRequiredZoneLength() const;
   void SetStartRequiredZoneLength(Float64 zl);
   const WBFL::Materials::Rebar* GetStartBar() const;
   void SetStartBar(const WBFL::Materials::Rebar* pRebar);
   Float64 GetStartS() const;
   void SetStartS(Float64 s);
   Float64 GetStartd() const; // beam depth
   void SetStartd(Float64 d);

   bool   StartPassed() const;

   Float64 GetEndProvidedZoneLength() const;
   void SetEndProvidedZoneLength(Float64 zl);
   Float64 GetEndRequiredZoneLength() const;
   void SetEndRequiredZoneLength(Float64 zl);
   const WBFL::Materials::Rebar* GetEndBar() const;
   void SetEndBar(const WBFL::Materials::Rebar* pRebar);
   Float64 GetEndS() const;
   void SetEndS(Float64 s);
   Float64 GetEndd() const;
   void SetEndd(Float64 d);
   bool   EndPassed() const;

   bool Passed() const;

protected:

private:
   bool m_IsApplicable;

   const WBFL::Materials::Rebar* m_pMinRebar;
   Float64 m_SMax;

   Float64 m_ZoneLengthFactor;

   Float64 m_StartProvidedZoneLength;
   Float64 m_StartRequiredZoneLength;
   const WBFL::Materials::Rebar* m_pStartRebar;
   Float64 m_StartS;
   Float64 m_Startd;

   Float64 m_EndProvidedZoneLength;
   Float64 m_EndRequiredZoneLength;
   const WBFL::Materials::Rebar* m_pEndRebar;
   Float64 m_EndS;
   Float64 m_Endd;
};
