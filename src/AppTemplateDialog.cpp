#include "AppTemplateDialog.h"

AppTemplateDialog::AppTemplateDialog() :
	WindowDialog(L"APPTEMPLATE", L"AppTemplate")
{
}

AppTemplateDialog::~AppTemplateDialog()
{
}

void AppTemplateDialog::OnInitDialog()
{
	DisableMaximize();
	DisableMinimize();
	DisableSize();
}


void AppTemplateDialog::OnDestroy()
{

}

void AppTemplateDialog::OnPaint()
{

}