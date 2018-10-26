#pragma once

#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignBrokerRetreiver.h"

#include "TogaSectionCutDrawStrategy.h"
#include "resource.h"

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
   afx_msg void OnFilePrint();
   virtual void AssertValid() const;

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

private:
   bool m_bCutLocationInitialized;

   Float64 m_CurrentCutLocation;
   CutLocation m_CutLocation;
   Float64 m_MaxCutLocation;

};
