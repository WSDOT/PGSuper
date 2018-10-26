#pragma once

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

// CTxDOTOptionalDesignBridgeInputPage dialog

class CTxDOTOptionalDesignBridgeInputPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignBridgeInputPage)

public:
	CTxDOTOptionalDesignBridgeInputPage();
	virtual ~CTxDOTOptionalDesignBridgeInputPage();

// Dialog Data
	enum { IDD = IDD_BRIDGE_INPUT_PAGE };

   CString m_Bridge;
   CString m_BridgeID;
   CString m_JobNumber;
   CString m_Engineer;
   CString m_Company;
   CString m_Comments;

   int m_SpanNo;
   int m_BeamNo;
   CString m_BeamType;
   Float64 m_BeamSpacing;
   Float64 m_SpanLength;
   Float64 m_SlabThickness;
   Float64 m_RelativeHumidity;
   Float64 m_LldfMoment;
   Float64 m_LldfShear;

   Float64 m_EcSlab;
   Float64 m_EcBeam;
   Float64 m_FcSlab;

   Float64 m_Ft;
   Float64 m_Fb;
   Float64 m_Mu;

   Float64 m_WNonCompDc;
   Float64 m_WCompDc;
   Float64 m_WCompDw;

   // Store a pointer to our data source
   CTxDOTOptionalDesignData* m_pData;

   // here we can get our broker
   ITxDOTBrokerRetriever* m_pBrokerRetriever;

private:
   void LoadDialogData();
   void SaveDialogData();
   void LoadGirderNames();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
