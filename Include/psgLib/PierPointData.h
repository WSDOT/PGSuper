///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include "PsgLibLib.h"
#include <vector>

class CPierData2;

class PSGLIBCLASS CPierPointData
{
public:

   CPierPointData(CPierData2* pPier = nullptr);
   CPierPointData(const CPierPointData& rOther); 
   virtual ~CPierPointData();

   CPierPointData& operator = (const CPierPointData& rOther);

   bool operator==(const CPierPointData& rOther) const;
   bool operator!=(const CPierPointData& rOther) const;

   void SetPier(CPierData2* pPier);
   const CPierData2* GetPier() const;
   CPierData2* GetPier();

   // Each transition connects the previous point to this point.
   enum TransitionType { ttStraight = 0, ttArcUpward = 1, ttArcDownward = 2 };
   void SetTransition(TransitionType type, Float64 radius);
   TransitionType GetTransitionType() const;
   Float64 GetRadius() const;

   bool IsEndPoint() const { return m_IsEndPoint; }
   void SetEndPoint(bool endPoint) { m_IsEndPoint = endPoint; }
   static void UpdateEndPoint(std::vector<CPierPointData>& points, Float64 length, Float64 depth);

   void Set_X(Float64 ppX);
   Float64 Get_X() const;

   void Set_Y(Float64 ppY);
   Float64 Get_Y() const;

	HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress);
	HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);

#if defined _DEBUG
   void AssertValid() const;
#endif

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const CPierPointData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CPierPointData& rOther);

private:
   CPierData2* m_pPier;
   Float64 m_X;
   Float64 m_Y;
   TransitionType m_TransitionType = ttStraight;
   Float64 m_Radius = 0;
   bool m_IsEndPoint = false;
};
