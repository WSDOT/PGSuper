///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
   CCastLongitudinalJointActivity

DESCRIPTION
   Encapsulates the data for the cast longitudinal joint activity
*****************************************************************************/

class PGSEXTCLASS CCastLongitudinalJointActivity
{
public:
   CCastLongitudinalJointActivity();
   CCastLongitudinalJointActivity(const CCastLongitudinalJointActivity& rOther);
   ~CCastLongitudinalJointActivity();

   CCastLongitudinalJointActivity& operator= (const CCastLongitudinalJointActivity& rOther);
   bool operator==(const CCastLongitudinalJointActivity& rOther) const;
   bool operator!=(const CCastLongitudinalJointActivity& rOther) const;

   void Enable(bool bEnable = true);
   bool IsEnabled() const;

   void SetConcreteAgeAtContinuity(Float64 age);
   Float64 GetConcreteAgeAtContinuity() const;

   void SetCuringDuration(Float64 duration);
   Float64 GetCuringDuration() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CCastLongitudinalJointActivity& rOther);
   void MakeAssignment(const CCastLongitudinalJointActivity& rOther);
   bool m_bEnabled;
   
   Float64 m_Age;
   Float64 m_CuringDuration;
};
