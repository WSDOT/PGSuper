#pragma once

#include "LinearDuctGrid.h"
#include "SplicedGirderGeneralPage.h"

// CLinearDuctDlg dialog

class CLinearDuctDlg : public CDialog, public CLinearDuctGridCallback
{
	DECLARE_DYNAMIC(CLinearDuctDlg)

public:
	CLinearDuctDlg(CSplicedGirderGeneralPage* pGdrDlg, CPTData* pPTData,DuctIndexType ductIdx,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CLinearDuctDlg();

   void EnableDeleteBtn(BOOL bEnable);

   const CGirderKey& GetGirderKey() const;

   const CLinearDuctGeometry& GetDuctGeometry() const;

   WBFL::Graphing::PointMapper::MapMode GetTendonControlMapMode() const;

// Dialog Data
	enum { IDD = IDD_LINEAR_DUCT };

   // returns the current value of the measurement type. only use when the dialog is open
   CLinearDuctGeometry::MeasurementType GetMeasurementType();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CPTData m_PTData;
   DuctIndexType m_DuctIdx;
   CLinearDuctGrid m_Grid;
   CSplicedGirderGeneralPage* m_pGirderlineDlg;
   CDrawTendonsControl m_DrawTendons;

   int m_PrevMeasurmentTypeIdx;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnAddPoint();
   afx_msg void OnDeletePoint();
   afx_msg void OnMeasurementTypeChanging();
   afx_msg void OnMeasurementTypeChanged();
   afx_msg void OnHelp();
   afx_msg void OnSchematicButton();

   virtual void OnDuctChanged();
};
