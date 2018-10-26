#pragma once

#include "PGSuperAppPlugin\resource.h"
#include "GirderGrid.h"
#include "DuctGrid.h"
#include <PgsExt\BridgeDescription2.h>
#include <Material\PsStrand.h>

#include "DrawTendonsControl.h"

// CEditGirderlineDlg dialog

class CEditGirderlineDlg : public CDialog, public IGirderSegmentDataSource
{
	DECLARE_DYNAMIC(CEditGirderlineDlg)

public:
	CEditGirderlineDlg(const CSplicedGirderData* pGirder,CWnd* pParent = NULL);   // standard constructor
	virtual ~CEditGirderlineDlg();

// Dialog Data
	enum { IDD = IDD_GIRDERLINE };

   CGirderKey m_GirderKey;
   GirderIDType m_GirderID;
   mutable CSplicedGirderData m_Girder;
   std::vector<EventIndexType> m_TendonStressingEvent; // index is duct index, value is event when tendon is stressed
   std::vector<EventIndexType> m_CastClosureEvent;

   bool m_bCopyToAll;

   int GetDuctCount();

   const matPsStrand* GetStrand();

   // IGirderSegmentDataSource
   virtual const CSplicedGirderData* GetGirder();
   virtual const CGirderKey& GetGirderKey();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void FillGirderComboBox();
   void FillStrandList(UINT nIDC);
   void FillStrandList(CComboBox* pList,matPsStrand::Grade grade,matPsStrand::Type type);
   void SetStrand();


   CGirderGrid m_GirderGrid;
   CDuctGrid   m_DuctGrid;
   CDrawTendonsControl m_DrawTendons;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnAddDuct();
   afx_msg void OnDeleteDuct();
   afx_msg void OnStrandSizeChanged();
   afx_msg void OnStrandChanged();
   afx_msg void OnConditionFactorTypeChanged();

   void OnDuctChanged();
   afx_msg void OnHelp();
};
