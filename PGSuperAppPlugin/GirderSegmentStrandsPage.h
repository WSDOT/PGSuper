#pragma once

#include "DebondGrid.h"
#include <GraphicsLib\GraphicsLib.h>

struct IStrandGeometry;

// CGirderSegmentStrandsPage dialog

class CGirderSegmentStrandsPage : public CPropertyPage, public IDebondGridParent
{
	DECLARE_DYNAMIC(CGirderSegmentStrandsPage)

public:
	CGirderSegmentStrandsPage();
	virtual ~CGirderSegmentStrandsPage();

	int		m_StrandSizeIdx;
	BOOL	m_bSymmetricDebond;

// Dialog Data
	enum { IDD = IDD_SEGMENT_STRANDS };

   Float64 GetMaxPjack(StrandIndexType nStrands); // allowable by spec
   Float64 GetUltPjack(StrandIndexType nStrands); // breaking strength

   // IDebondGridParent
   virtual LPCTSTR GetGirderName();
   virtual void OnChange();
   virtual const CSegmentKey& GetSegmentKey();
   virtual ConfigStrandFillVector ComputeStrandFillVector(pgsTypes::StrandType type);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//{{AFX_MSG(CGirderSegmentStrandsPage)
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	afx_msg void OnNumHarpedStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNumStraightStrandsChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateHarpedStrandPjEdit();
	afx_msg void OnUpdateStraightStrandPjEdit();
	afx_msg void OnHelp();
   afx_msg void OnStrandTypeChanged();
	afx_msg void OnSymmetricDebond();
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   StrandIndexType OnNumStrandsChanged(UINT nCheck,UINT nEdit,UINT nUnit,pgsTypes::StrandType strandType,int iPos,int& iDelta);
   void OnUpdateStrandPjEdit(UINT nCheck,UINT nStrandEdit,UINT nForceEdit,UINT nUnit,pgsTypes::StrandType strandType);

   void UpdateStrandControls();
   void UpdateStrandList(UINT nIDC);

   void UpdateGrid();

   void InitPjackEdits();
   void InitPjackEdits(UINT nCalcPjack,UINT nNumStrands,UINT nPjackEdit,UINT nPjackUnit,pgsTypes::StrandType strandType);

   StrandIndexType StrandSpinnerInc(IStrandGeometry* pStrands, pgsTypes::StrandType type,StrandIndexType currNum, bool bAdd );

   void DrawShape(CDC* pDC,IShape* shape,grlibPointMapper& mapper);
   void DrawStrands(CDC* pDC,grlibPointMapper& mapper);

   CComPtr<IIndexArray> m_Debondables;

   CGirderDescDebondGrid m_Grid;

   Float64 m_HgStart;
   Float64 m_HgHp1;
   Float64 m_HgHp2;
   Float64 m_HgEnd;
};
