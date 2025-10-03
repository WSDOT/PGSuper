///////////////////////////////////////////////////////////////////////
// IEPluginExample
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

// PGSuperExporter.h : Declaration of the CPGSuperExporter
#pragma once

#include <Plugins\PGSuperIEPlugin.h>
#include <EAF\ComponentObject.h>
#include <MfcTools/ExcelWrapper.h>
#include <EAF/AutoProgress.h>

class CGirderScheduleExporter : public WBFL::EAF::ComponentObject,
	public PGS::IDataExporter
{
public:
	CGirderScheduleExporter();

	STDMETHOD(Init)(UINT nCmdID) override;
	CString GetMenuText() const override;
	HBITMAP GetBitmapHandle() const override;
	CString GetCommandHintText() const override;
	HRESULT Export(std::shared_ptr<WBFL::EAF::Broker> pBroker) override;

private:

	struct ScheduleHeaderInfo
	{
		ColumnIndexType colIdx;
		ColumnIndexType colSpan; 
		RowIndexType rowIdx;
		RowIndexType rowSpan; 
		Float64 orientation;
		CString strValue;
	};

	CString GetColumnLabel(ColumnIndexType colIdx);
	void SetColumnHeader(_Worksheet* worksheet, ColumnIndexType colIdx, ColumnIndexType colSpan, 
		RowIndexType rowIdx, RowIndexType rowSpan, Float64 orientation, CString strValue);
	void SetColumnData(_Worksheet* pWorksheet, ColumnIndexType colIdx, RowIndexType rowIdx, CString strValue);
	void SetColumnData(_Worksheet* pWorksheet, ColumnIndexType colIdx, RowIndexType rowIdx, Float64 value);
	void SetColumnData(_Worksheet* pWorksheet, ColumnIndexType colIdx, RowIndexType rowIdx, rptReportContent& rptItem);
	bool DoesFileExist(const CString& filname);
	bool CommitExcel(_Application& excel, Worksheets& worksheets, LPCTSTR strFilename);

	CBitmap m_Bitmap;
};
