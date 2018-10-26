#pragma once



// CTxDOTOptionalDesignPropertySheet

class CTxDOTOptionalDesignPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CTxDOTOptionalDesignPropertySheet)

public:
	CTxDOTOptionalDesignPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CTxDOTOptionalDesignPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CTxDOTOptionalDesignPropertySheet();

protected:
	DECLARE_MESSAGE_MAP()
public:
   afx_msg void OnSize(UINT nType, int cx, int cy);
   virtual BOOL OnInitDialog();

private:
   int m_TabHeight;
};


