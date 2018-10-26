///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include "resource.h"
#include <Material\Rebar.h>

class CShearBarsLegsGrid;
// CShearDesignPage dialog

class CShearDesignPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CShearDesignPage)

   friend CShearBarsLegsGrid;

public:
	CShearDesignPage();
	virtual ~CShearDesignPage();

   virtual BOOL OnInitDialog();
   afx_msg void OnHelp();

   void OnEnableDelete(bool canDelete);
   void DoRemoveRows();
   void DoInsertRow();
   afx_msg void OnRemoveRows();
   afx_msg void OnInsertRow();

   // Dialog Data
	enum { IDD = IDD_SHEAR_DESIGN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:

   struct StirrupSizeBarCombo
   {
      matRebar::Size Size;
      Float64 NLegs;

      bool operator==(const StirrupSizeBarCombo& rOther) const
      {
         if (!::IsEqual(NLegs, rOther.NLegs))
            return false;

         return Size==rOther.Size;
      }
   };

   typedef std::vector<StirrupSizeBarCombo> StirrupSizeBarComboColl;
   typedef StirrupSizeBarComboColl::iterator StirrupSizeBarComboIter;
   typedef StirrupSizeBarComboColl::const_iterator StirrupSizeBarComboConstIter;

   StirrupSizeBarComboColl m_StirrupSizeBarComboColl;

   std::vector<Float64> m_BarSpacings;

   Float64 m_MaxStirrupSpacingChange;
   Float64 m_MaxShearCapChange;

   int m_MinZoneLengthBars;
   Float64 m_MinZoneLengthDist;

   BOOL m_bExtendDeckBars;
   BOOL m_bBarsProvideConfinement;

   int m_LongReinfShearMethod;

private:
   std::auto_ptr<CShearBarsLegsGrid> m_pGrid;
};
