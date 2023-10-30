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

/*****************************************************************************
CLASS 
   CGeometryControlActivity

DESCRIPTION
   Encapsulates the data for the geometry control activity.
*****************************************************************************/

class PGSEXTCLASS CGeometryControlActivity
{
public:
   CGeometryControlActivity();
   CGeometryControlActivity(const CGeometryControlActivity& rOther);
   ~CGeometryControlActivity();

   CGeometryControlActivity& operator= (const CGeometryControlActivity& rOther);
   bool operator==(const CGeometryControlActivity& rOther) const;
   bool operator!=(const CGeometryControlActivity& rOther) const;

   bool IsEnabled() const;

   void Clear();

   // Set/Get pgsTypes::GeometryControlActivityType
   void SetGeometryControlEventType(pgsTypes::GeometryControlActivityType type);
   pgsTypes::GeometryControlActivityType GetGeometryControlEventType() const;

   // If true, this is the activity where roadway geometry determines elevations of ends of segment chords
   bool IsGeometryControlEvent() const;

   // If true, create a roadway geometry spec check for this activity's interval
   bool IsSpecCheck() const;

   // Report roadway goemetry information. Don't necessarily do a spec check
   bool IsReport() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CGeometryControlActivity& rOther);
   void MakeAssignment(const CGeometryControlActivity& rOther);

   void Update();

   pgsTypes::GeometryControlActivityType m_ActivityType;
};
