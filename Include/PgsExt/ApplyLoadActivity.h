///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
   CApplyLoadActivity

DESCRIPTION
   Encapsulates the data for the apply load activity
*****************************************************************************/

class PGSEXTCLASS CApplyLoadActivity
{
public:
   CApplyLoadActivity();
   CApplyLoadActivity(const CApplyLoadActivity& rOther);
   ~CApplyLoadActivity();

   CApplyLoadActivity& operator= (const CApplyLoadActivity& rOther);
   bool operator==(const CApplyLoadActivity& rOther) const;
   bool operator!=(const CApplyLoadActivity& rOther) const;

   void Enable(bool bEnable=true);
   bool IsEnabled() const;

   void Clear();

   void ApplyIntermediateDiaphragmLoad(bool bApplyLoad = true);
   bool IsIntermediateDiaphragmLoadApplied() const;

   void ApplyRailingSystemLoad(bool bApplyLoad = true);
   bool IsRailingSystemLoadApplied() const;

   void ApplyOverlayLoad(bool bApplyLoad = true);
   bool IsOverlayLoadApplied() const;

   void ApplyLiveLoad(bool bApplyLoad = true);
   bool IsLiveLoadApplied() const;

   void ApplyRatingLiveLoad(bool bApplyLoad = true);
   bool IsRatingLiveLoadApplied() const;

   void AddUserLoad(LoadIDType loadID);
   void RemoveUserLoad(LoadIDType loadID);
   bool HasUserLoad(LoadIDType loadID) const;
   IndexType GetUserLoadCount() const;
   LoadIDType GetUserLoadID(IndexType idx) const;
   const std::set<LoadIDType>& GetUserLoads() const;
   void SetUserLoads(const std::set<LoadIDType>& loads);
   bool IsUserLoadApplied() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CApplyLoadActivity& rOther);
   void MakeAssignment(const CApplyLoadActivity& rOther);

   void Update();

   bool m_bEnabled;

   bool m_bApplyIntermediateDiaphragmLoad;
   bool m_bApplyRailingSystemLoad;
   bool m_bApplyOverlayLoad;
   bool m_bApplyLiveLoad;
   bool m_bApplyRatingLiveLoad;

   std::set<LoadIDType> m_UserLoads;
};
