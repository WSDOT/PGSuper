#pragma once

#include "ParabolicDuctGrid.h"
#include "SplicedGirderGeneralPage.h"
#include "DrawTendonsControl.h"

// CParabolicDuctDlg dialog

class CParabolicDuctDlg : public CDialog, public CParabolicDuctGridCallback
{
	DECLARE_DYNAMIC(CParabolicDuctDlg)

public:
	CParabolicDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,CPTData* pPTData,DuctIndexType ductIdx,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CParabolicDuctDlg();


// Dialog Data
	enum { IDD = IDD_PARABOLIC_DUCT };

   const CParabolicDuctGeometry& GetDuctGeometry() const;

   grlibPointMapper::MapMode GetTendonControlMapMode() const;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void FillPierLists();

	DECLARE_MESSAGE_MAP()

   CSplicedGirderGeneralPage* m_pGirderlineDlg;
   CPTData m_PTData;
   DuctIndexType m_DuctIdx;
   CDrawTendonsControl m_DrawTendons;
   CParabolicDuctGrid m_Grid;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
   afx_msg void OnRangeChanged();
   afx_msg void OnSchematicButton();

   virtual void OnDuctChanged();
};
