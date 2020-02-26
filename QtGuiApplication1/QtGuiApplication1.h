#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtGuiApplication1.h"

class QtGuiApplication1 : public QMainWindow
{
	Q_OBJECT
public:
	QtGuiApplication1(QWidget *parent = Q_NULLPTR);
protected slots:
	void onProcess();
private:
	Ui::QtGuiApplication1Class ui;
	QImage _originalImage;
	QImage _oversegmentedImage;
	QImage _edgeImage;
};
