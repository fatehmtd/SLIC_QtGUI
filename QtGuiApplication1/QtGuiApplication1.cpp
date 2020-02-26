#include "QtGuiApplication1.h"
#include <QPushButton>
#include <QFileDialog>
#include <QCheckBox>
#include <QDebug>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include "SLIC.h"

QtGuiApplication1::QtGuiApplication1(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	connect(ui.btnBrowse, &QPushButton::clicked, [=]()
		{
			auto file = QFileDialog::getOpenFileName(this, "Select image file", "", "*.jpg;*.bmp;*.png;");
			if (_originalImage.load(file))
			{
				ui.txtPath->setText(file);
				ui.imageHolder->setPixmap(QPixmap::fromImage(_originalImage));

				_edgeImage = QImage();
				_oversegmentedImage = QImage();

				ui.btnReset->setEnabled(true);
				ui.btnSLIC->setEnabled(true);
				ui.valueM->setEnabled(true);
				ui.valueS->setEnabled(true);
				ui.valueC->setEnabled(true);
				ui.valueLoops->setEnabled(true);
			}
		}); // browse for image

	connect(ui.btnReset, &QPushButton::clicked, [=]() {ui.imageHolder->setPixmap(QPixmap::fromImage(_originalImage)); }); // reset image

	connect(ui.btnSLIC, &QPushButton::clicked, this, &QtGuiApplication1::onProcess);

	connect(ui.valueEdges, &QCheckBox::stateChanged, [=](int state) 
		{
			if (_oversegmentedImage.isNull()) return;
			ui.imageHolder->setPixmap(QPixmap::fromImage(!state ? _oversegmentedImage : _edgeImage));
		});
}

void QtGuiApplication1::onProcess()
{
	QProgressDialog progressDlg;
	progressDlg.reset();
	progressDlg.setRange(0, 0);
	progressDlg.setLabelText("Processing, please wait");
	progressDlg.setWindowTitle("SLIC");
	progressDlg.setCancelButton(nullptr);
	progressDlg.setAutoClose(true);

	QFutureWatcher<SLIC::SLICResult> _watcher;

	connect(&_watcher, &QFutureWatcherBase::finished, [=, &_watcher, &progressDlg]()
		{
			auto result = _watcher.result();
			_oversegmentedImage = result._oversegmentedImage;
			_edgeImage = result._edgesImage;

			ui.imageHolder->setPixmap(QPixmap::fromImage(!ui.valueEdges->isChecked() ? _oversegmentedImage : _edgeImage));
			progressDlg.cancel();
		});

	auto execSLic = [](float s, float m, float c, const QImage& image, int loop) -> SLIC::SLICResult
	{
		SLIC slic;
		return slic.execute(s, // S: controls the size of the clusters
			m, // M: controls the weight of the spatial distance
			c, // C: controls the weight of the color distance
			image, // the image to oversegment
			loop); // the number of loops
	};

	QFuture<SLIC::SLICResult> future = QtConcurrent::run(execSLic,
		ui.valueS->value(), // S: controls the size of the clusters
		ui.valueM->value(), // M: controls the weight of the spatial distance
		ui.valueC->value(), // C: controls the weight of the color distance
		_originalImage, // the image to oversegment
		ui.valueLoops->value());

	_watcher.setFuture(future);

	progressDlg.exec();
}
