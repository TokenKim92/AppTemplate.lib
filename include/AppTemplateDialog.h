#ifndef _THIN_PNG_DIALOG_H_
#define _THIN_PNG_DIALOG_H_

#include "WindowDialog.h"

class AppTemplateDialog : public WindowDialog
{
public:
	AppTemplateDialog();
	virtual ~AppTemplateDialog();

public:
	virtual void OnInitDialog() override;
	virtual void OnDestroy() override;
	virtual void OnPaint() override;
};

#endif //_THIN_PNG_DIALOG_H_