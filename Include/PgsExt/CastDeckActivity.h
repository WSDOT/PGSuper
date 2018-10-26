///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
   CCastDeckActivity

DESCRIPTION
   Encapsulates the data for the cast deck activity
*****************************************************************************/

class PGSEXTCLASS CCastDeckActivity
{
public:
   CCastDeckActivity();
   CCastDeckActivity(const CCastDeckActivity& rOther);
   ~CCastDeckActivity();

   CCastDeckActivity& operator= (const CCastDeckActivity& rOther);
   bool operator==(const CCastDeckActivity& rOther) const;
   bool operator!=(const CCastDeckActivity& rOther) const;

   void Enable(bool bEnable = true);
   bool IsEnabled() const;

   void SetConcreteAgeAtContinuity(Float64 age);
   Float64 GetConcreteAgeAtContinuity() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CCastDeckActivity& rOther);
   virtual void MakeAssignment(const CCastDeckActivity& rOther);
   bool m_bEnabled;
   
   Float64 m_Age;

   // NOTE: In the future we may want to model the deck being cast in segments
   // much like a steel bridge. To do this, add a start and end station to this
   // activity so we know where the deck is being cast
};
