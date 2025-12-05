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

	struct WarningInfo {
		CString msg;
		CString prop; // e.g., "Color" or "ColorIndex"
		VARIANT var;
		WarningInfo(const CString& m, const CString& p, long rgb) : msg(m), prop(p) {
			VariantInit(&var); var.vt = VT_I4; var.lVal = rgb;
		}
		WarningInfo(const CString& m, const CString& p, short idx) : msg(m), prop(p) {
			VariantInit(&var); var.vt = VT_I2; var.iVal = idx;
		}
		WarningInfo(const CString& m, const CString& p, const VARIANT& v) : msg(m), prop(p) {
			VariantInit(&var); VariantCopy(&var, const_cast<VARIANT*>(&v));
		}
		~WarningInfo() { VariantClear(&var); }
	};


	struct ScheduleRowData {
		CGirderKey girderKey;
		CString girderSeries; // girder series
		Float64 topWidth = 0; // top flange width
		Float64 Hg = 0; //gross section depth
		Float64 planLength = 0; //plan length
		Float64 nVoids = 0; //number of voids
		Float64 ExtVoidDiameter = 0; //exterior void diameter
		Float64 IntVoidDiameter = 0; //interior void diameter
		Float64 t1 = 0; // girder skew end 1
		Float64 t2 = 0; // girder skew end 2
		Float64 P1 = 0; // girder end 1 distance to bearing line
		Float64 P2 = 0; // girder end 2 distance to bearing line
		Float64 fc = 0; // 28-day strength
		Float64 fci = 0; // release strength
		StrandIndexType Ns = 0; // number of straight strands
		StrandIndexType Nh = 0; // number of harped strands
		StrandIndexType Nt = 0; // number of temporary strands
		std::array<StrandIndexType, 3> nStrandsInRows = { 0, 0, 0 }; // number of strands per row
		StrandIndexType nExtendedL = 0; // number of extended strands left
		StrandIndexType nExtendedR = 0; // number of extended strands right
		std::array<std::map<Float64, StrandIndexType>, 2> nDebondedPerLength; // number of de-bonded strands per de-bonded length
		Float64 E = 0; // location of straight strand C.G. from girder neutral axis at midspan
		Float64 Fcl = 0; // location of harped strand C.G. from girder neutral axis at midspan
		Float64 Fo = 0; // location of harped strand C.G. from girder neutral axis at end
		Float64 A = 0; // uniform design haunch
		Float64 Aend1 = 0; //design haunch at end 1
		Float64 Aend2 = 0; //design haunch at end 2
		Float64 C = 0; // maximum screed camber at midspan
		Float64 DminLowerBound = 0; //Minimum lower bound camber D40
		Float64 DmaxUpperBound = 0; //Maximum upper bound camber D120
		Float64 z1Length = 0; //shear zone 1 length
		Float64 z1Spacing = 0; //shear zone 1 transverse bar spacing
		Float64 z2Length = 0; //shear zone 2 length
		Float64 z2Spacing = 0; //shear zone 2 transverse bar spacing
		Float64 z3Length = 0; //shear zone 3 length
		Float64 z3Spacing = 0; //shear zone 3 transverse bar spacing
		WBFL::Materials::Rebar::Size z1Size; //zone 1 transverse bar size
		WBFL::Materials::Rebar::Size z2Size; //zone 2 transverse bar size
		WBFL::Materials::Rebar::Size z3Size; //zone 3 transverse bar size
		Float64 H1 = 0; // uniform stirrup height
		Float64 H1end1 = 0; // stirrup height end 1
		Float64 H1end2 = 0; // stirrup height end 2
		std::vector<WBFL::Materials::Rebar::Size> vG1LongBarSize; //Top face (G1) longitudinal rebar size
		std::vector<IndexType> vG1NumLongBars; //Top face (G1) number of longitudinal bars
		std::vector<WBFL::Materials::Rebar::Size> vG2LongBarSize; //Top face (G2) longitudinal rebar size
		std::vector<IndexType> vG2NumLongBars; //Top face (G2) number of longitudinal bars
		Float64 midspanDeflection = 0; // Maximum midspan vertical deflection at shipping
		Float64 liftingLoopLocation = 0; // lifting loop location from the end
		Float64 trailingOverhang = 0; // trailing overhang during shipping
		Float64 leadingOverhang = 0; // leading overhang during shipping
		Float64 rollStiffness = 0; // hauling truck stiffness
		Float64 wheelSpacing = 0; //hauling truck wheel spacing
	};

	void AddDesignerNudges();


	CString GetColumnLabel(ColumnIndexType colIdx);
	void SetColumnHeader(_Worksheet* worksheet, ColumnIndexType colIdx, ColumnIndexType colSpan, 
		RowIndexType rowIdx, RowIndexType rowSpan, Float64 orientation, CString strValue);
	template <typename T>
	void SetColumnData(_Worksheet* pWorksheet, ColumnIndexType colIdx, RowIndexType rowIdx, T tValue);
	bool DoesFileExist(const CString& filname);
	std::string FormatFeetInchesFromDecimalInches(double totalInches, int denom);
	bool CommitExcel(_Application& excel, Worksheets& worksheets, LPCTSTR strFilename);

	CBitmap m_Bitmap;

	std::vector<ScheduleHeaderInfo> m_HeaderInfo;

	std::vector<CString> m_current_row_data;
	std::vector<CString> m_previous_row_data;

	std::vector<ScheduleRowData> m_schedule_data;

	GirderIndexType m_last_same_gdrID;

	std::vector<WarningInfo> m_warnings;
	std::vector<WarningInfo> m_optimizations;
};
