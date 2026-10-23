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
#include "XBeamCutLocation.h"
#include <Graphing/PointMapper.h>
#include <PsgLib\PierData2.h>
#include <PsgLib\Keys.h>
#include <DManip/DManip.h>

interface IPierLayoutDataSource
{
public:
   virtual const CPierData2 * GetPierData() const = 0;
   virtual void SetPierData(const CPierData2& pierData) = 0;
   virtual void ChangePierLayout() = 0;
};


class CDrawPierLayoutControl : public CDisplayWnd
{
	DECLARE_DYNAMIC(CDrawPierLayoutControl)

	friend class CPierLayoutPage;
	friend class CPierConnectionsPage;
	friend class CCommonPierLayoutDlg;
	friend class CScallopedPierLayoutDlg;
	friend class CUserDefinedPierLayoutDlg;

public:
	CDrawPierLayoutControl();
	virtual ~CDrawPierLayoutControl();

	void OnDraw(CDC* pDC) override;

	void CustomInit(IPierLayoutDataSource* pSource);

	BOOL CreatePopout(IPierLayoutDataSource* pSource, CWnd* pOwner);

protected:

	IDType m_DisplayObjectID; // used to generate display object IDs

	void UpdateDisplayObjects();
	void UpdateRoadwayDisplayObjects();
	void UpdateXBeamDisplayObjects();
	void UpdateColumnDisplayObjects();
	void UpdateDimensionsDisplayObjects();
	void UpdateSectionCutDisplayObjects();

	std::shared_ptr<WBFL::DManip::iLineDisplayObject> CreateLineDisplayObject(const WBFL::Geometry::Point2d& pntStart, const WBFL::Geometry::Point2d& pntEnd);
	void BuildDimensionLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint, bool bOmitForZeroDistance = true);
	void BuildDimensionLine(std::shared_ptr<WBFL::DManip::iDisplayList> pDL, const WBFL::Geometry::Point2d& fromPoint, const WBFL::Geometry::Point2d& toPoint, Float64 dimension);


	afx_msg void OnPaint();

	DECLARE_MESSAGE_MAP()

private:
	IPierLayoutDataSource* m_pSource;

	CXBeamCutLocation* m_pCutLoc;
};
