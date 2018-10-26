#pragma once

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"
#include "TogaSectionCutDrawStrategy.h"

#include "resource.h"
#include "afxwin.h"

class CTogaGirderModelElevationView;
class CTogaGirderModelSectionView;
class CTxDOTOptionalDesignDoc;

// CTxDOTOptionalDesignGirderViewPage dialog

class CTxDOTOptionalDesignGirderViewPage : public CPropertyPage, public ITxDataObserver, public iCutLocation
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignGirderViewPage)

public:
	CTxDOTOptionalDesignGirderViewPage();
	virtual ~CTxDOTOptionalDesignGirderViewPage();

// Dialog Data
	enum { IDD = IDD_GIRDER_VIEW_PAGE };

   // here we store a pointer to our data source and all change events until we need to display
   CTxDOTOptionalDesignData* m_pData;
   int m_ChangeStatus;

   // here we can get our broker
   ITxDOTBrokerRetriever* m_pBrokerRetriever;

// listen to data change events
   virtual void OnTxDotDataChanged(int change);

   // our views need the document
   CTxDOTOptionalDesignDoc* m_pDocument;

   SpanIndexType span;
   GirderIndexType girder;

   void GetSpanAndGirderSelection(SpanIndexType* pSpan,GirderIndexType* pGirder);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnSetActive();
   virtual BOOL OnInitDialog();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   virtual void AssertValid() const;
   afx_msg void OnSize(UINT nType, int cx, int cy);

   // iCutLocation
public:
   enum CutLocation {LeftEnd,LeftHarp,Center,RightHarp,RightEnd,UserInput};

   // Girder view
   void InvalidateCutLocation() {m_bCutLocationInitialized = false;}
   Float64 GetCurrentCutLocation() {return m_CurrentCutLocation;}
   void CutAt(Float64 cut);
   void ShowCutDlg();
   void CutAtLocation();
   void CutAtLeftEnd();
   void CutAtLeftHp();
   void CutAtCenter();
   void CutAtRightHp(); 
   void CutAtRightEnd(); 

   void CutAtNext();
   void CutAtPrev();

   void UpdateCutLocation(CutLocation cutLoc,Float64 cut = 0.0);
   void UpdateBar();

   void DisplayErrorMode(TxDOTBrokerRetrieverException& exc);

private:
   bool m_bCutLocationInitialized;

   Float64 m_CurrentCutLocation;
   CutLocation m_CutLocation;
   Float64 m_MaxCutLocation;
   GirderIndexType m_SelectedGirder;

public:
   CTogaGirderModelSectionView*   m_pSectionView;
   CTogaGirderModelElevationView* m_pElevationView;
   afx_msg void OnCbnSelchangeSelectedGirder();
   afx_msg void OnBnClickedSectionCut();
   CStatic m_ErrorMsgStatic;
   CComboBox m_GirderCtrl;
   CButton m_SectionBtn;
   afx_msg void OnViewSectioncutlocation();
   afx_msg void OnHelpFinder();
};
