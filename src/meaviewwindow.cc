/*! \file meaviewwindow.cc
 *
 * Implementation of main window for the meaview application.
 *
 * (C) 2016 Benjamin Naecker bnaecker@stanford.edu
 */

#include "meaviewwindow.h"
#include "configwindow.h"

namespace meaview {
namespace meaviewwindow {

MeaviewWindow::MeaviewWindow(QWidget* parent) :
	QMainWindow(parent),
	playbackStatus(PlaybackStatus::Paused),
	lastDataReceived(0.),
	lastDataPlotted(0.)
{
	setWindowTitle("meaview");
	setGeometry(WindowPosition.first, WindowPosition.second,
			WindowSize.first, WindowSize.second);
	createDockWidgets();
	initSettings();
	initMenus();
	initServerWidget();
	initPlaybackControlWidget();
	initDisplaySettingsWidget();
	initPlotWindow();
	requestTimer = new QTimer(this);
	requestTimer->setInterval(settings.value("display/refresh").toInt());
	initSignals();
	statusBar()->showMessage("Ready", StatusMessageTimeout);
}

MeaviewWindow::~MeaviewWindow()
{
	if (client) {
		client->disconnect();
		client->deleteLater();
	}
}

void MeaviewWindow::readConfigurationFile()
{
}

void MeaviewWindow::initSettings() 
{
	settings.setValue("display/scale", plotwindow::McsDefaultDisplayRange);
	settings.setValue("display/scale-multiplier", 1.0); // start with Volts
	settings.setValue("display/refresh",
			plotwindow::DefaultRefreshInterval);
	settings.setValue("display/view",
			plotwindow::DefaultChannelView);
	settings.setValue("display/autoscale", false);
	settings.setValue("data/request-size", meaviewwindow::DataRequestChunkSize);
}

void MeaviewWindow::createDockWidgets() 
{
	serverWidget = new QWidget(this);
	serverWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	serverDockWidget = new QDockWidget("Server", this);

	playbackControlWidget = new QWidget(this);
	playbackControlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	playbackControlDockWidget = new QDockWidget("Playback", this);

	displaySettingsWidget = new QWidget(this);
	displaySettingsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	displaySettingsDockWidget = new QDockWidget("Display", this);

}

void MeaviewWindow::initMenus() 
{
	menuBar = new QMenuBar(nullptr);

	serverMenu = new QMenu(tr("&Server"));

	connectToDataServerAction = new QAction("&Connect to server", serverMenu);
	connectToDataServerAction->setShortcut(QKeySequence("Ctrl+C"));
	connectToDataServerAction->setCheckable(false);
	QObject::connect(connectToDataServerAction, &QAction::triggered,
			this, &MeaviewWindow::connectToDataServer);
	serverMenu->addAction(connectToDataServerAction);

	disconnectFromDataServerAction = new QAction("&Disconnect from server", serverMenu);
	disconnectFromDataServerAction->setShortcut(QKeySequence("Ctrl+D"));
	disconnectFromDataServerAction->setCheckable(false);
	disconnectFromDataServerAction->setEnabled(false);
	QObject::connect(disconnectFromDataServerAction, &QAction::triggered,
			this, &MeaviewWindow::disconnectFromDataServer);
	serverMenu->addAction(disconnectFromDataServerAction);

	menuBar->addMenu(serverMenu);

	playbackMenu = new QMenu(tr("&Playback"));

	startPlaybackAction = new QAction(tr("&Start"), playbackMenu);
	startPlaybackAction->setShortcut(QKeySequence(Qt::Key_Space));
	startPlaybackAction->setCheckable(false);
	startPlaybackAction->setEnabled(false);
	playbackMenu->addAction(startPlaybackAction);

	jumpBackwardAction = new QAction(tr("Jump &backward"), playbackMenu);
	jumpBackwardAction->setShortcut(QKeySequence(Qt::Key_Left));
	jumpBackwardAction->setCheckable(false);
	jumpBackwardAction->setEnabled(false);
	QObject::connect(jumpBackwardAction, &QAction::triggered,
			this, &MeaviewWindow::jumpBackward);
	playbackMenu->addAction(jumpBackwardAction);

	jumpForwardAction = new QAction(tr("Jump &forward"), playbackMenu);
	jumpForwardAction->setShortcut(QKeySequence(Qt::Key_Right));
	jumpForwardAction->setCheckable(false);
	jumpForwardAction->setEnabled(false);
	QObject::connect(jumpForwardAction, &QAction::triggered,
			this, &MeaviewWindow::jumpForward);
	playbackMenu->addAction(jumpForwardAction);

	jumpToStartAction = new QAction(tr("Jump to &start"), playbackMenu);
	jumpToStartAction->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Left));
	jumpToStartAction->setCheckable(false);
	jumpToStartAction->setEnabled(false);
	QObject::connect(jumpToStartAction, &QAction::triggered,
			this, &MeaviewWindow::jumpToStart);
	playbackMenu->addAction(jumpToStartAction);

	jumpToEndAction = new QAction(tr("Jump to &end"), playbackMenu);
	jumpToEndAction->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Right));
	jumpToEndAction->setCheckable(false);
	jumpToEndAction->setEnabled(false);
	QObject::connect(jumpToEndAction, &QAction::triggered,
			this, &MeaviewWindow::jumpToEnd);
	playbackMenu->addAction(jumpToEndAction);

	menuBar->addMenu(playbackMenu);

	viewMenu = new QMenu(tr("&View"));

	showServerDockWidget = serverDockWidget->toggleViewAction();
	viewMenu->addAction(showServerDockWidget);

	showPlaybackControlDockWidget = playbackControlDockWidget->toggleViewAction();
	viewMenu->addAction(showPlaybackControlDockWidget);

	showDisplaySettingsDockWidget = displaySettingsDockWidget->toggleViewAction();
	viewMenu->addAction(showDisplaySettingsDockWidget);

	viewMenu->addSeparator();

	showInspectorsAction = new QAction(tr("&Inspectors"), viewMenu);
	showInspectorsAction->setShortcut(QKeySequence("Ctrl+I"));
	showInspectorsAction->setEnabled(false);
	showInspectorsAction->setCheckable(true);
	showInspectorsAction->setChecked(false);
	viewMenu->addAction(showInspectorsAction);

	showHidensConfigurationAction = new QAction(tr("Show &HiDens configuration"), viewMenu);
	showHidensConfigurationAction->setShortcut(QKeySequence("Ctrl+H"));
	showHidensConfigurationAction->setEnabled(false);
	showHidensConfigurationAction->setCheckable(false);
	QObject::connect(showHidensConfigurationAction, &QAction::triggered,
			this, &MeaviewWindow::showHidensConfiguration);
	viewMenu->addAction(showHidensConfigurationAction);

	minifyAction = new QAction(tr("&Minify"), viewMenu);
	minifyAction->setShortcut(QKeySequence("Ctrl+M"));
	minifyAction->setEnabled(true);
	minifyAction->setCheckable(true);
	minifyAction->setChecked(false);
	QObject::connect(minifyAction, &QAction::triggered,
			this, &MeaviewWindow::minify);
	//viewMenu->addAction(minifyAction); // not implementing minification ability yet

	menuBar->addMenu(viewMenu);

	setMenuBar(menuBar);
}

void MeaviewWindow::initServerWidget() 
{
	serverLabel = new QLabel("Server:", this);
	serverLabel->setAlignment(Qt::AlignRight);

	serverLine = new QLineEdit(DefaultServerHost, this);
	serverLine->setToolTip("The hostname or IP address of the data server");

	connectToDataServerButton = new QPushButton("Connect", this);
	connectToDataServerButton->setToolTip("Connect to the requested data server application");
	QObject::connect(connectToDataServerButton, &QPushButton::clicked,
			connectToDataServerAction, &QAction::trigger);

	serverLayout = new QGridLayout();
	serverLayout->addWidget(serverLabel, 0, 0);
	serverLayout->addWidget(connectToDataServerButton, 0, 1);
	serverLayout->addWidget(serverLine, 1, 0, 1, 2);

	serverWidget->setLayout(serverLayout);
	serverDockWidget->setFloating(false);
	serverDockWidget->setWidget(serverWidget);
	addDockWidget(Qt::TopDockWidgetArea, serverDockWidget);
}

void MeaviewWindow::initPlaybackControlWidget() 
{
	timeLabel = new QLabel("Current time:", playbackControlWidget);
	timeLabel->setAlignment(Qt::AlignRight);
	timeLine = new QLineEdit("0", playbackControlWidget);
	timeLine->setReadOnly(true);
	timeLine->setToolTip("Current time in the recording");
	
	totalTimeLabel = new QLabel("Total time:", playbackControlWidget);
	totalTimeLabel->setAlignment(Qt::AlignRight);
	totalTimeLine = new QLineEdit("0", playbackControlWidget);
	totalTimeLine->setToolTip("Total time in the recording");
	totalTimeLine->setReadOnly(true);
	
	jumpToStartButton = new QPushButton("Start", playbackControlWidget);
	jumpToStartButton->setToolTip("Jump back to the start of this recording");
	jumpToStartButton->addAction(jumpToStartAction);
	jumpToStartButton->setEnabled(false);
	QObject::connect(jumpToStartButton, &QPushButton::clicked,
			jumpToStartAction, &QAction::trigger);
	
	jumpBackwardButton = new QPushButton("Back", playbackControlWidget);
	jumpBackwardButton->setToolTip("Skip backwards in this recording");
	jumpBackwardButton->addAction(jumpBackwardAction);
	jumpBackwardButton->setEnabled(false);
	QObject::connect(jumpBackwardButton, &QPushButton::clicked,
			jumpBackwardAction, &QAction::trigger);

	startPlaybackButton = new QPushButton("Play", playbackControlWidget);
	startPlaybackButton->setToolTip("Start or pause plotting of the current recording");
	startPlaybackButton->addAction(startPlaybackAction);
	startPlaybackButton->setEnabled(false);
	QObject::connect(startPlaybackButton, &QPushButton::clicked,
			startPlaybackAction, &QAction::trigger);

	jumpForwardButton = new QPushButton("Forward", playbackControlWidget);
	jumpForwardButton->setToolTip("Skip forwards in this recording");
	jumpForwardButton->addAction(jumpForwardAction);
	jumpForwardButton->setEnabled(false);
	QObject::connect(jumpForwardButton, &QPushButton::clicked,
			jumpForwardAction, &QAction::trigger);

	jumpToEndButton = new QPushButton("End", playbackControlWidget);
	jumpToEndButton->setToolTip("Jump to the most recent data in this recording");
	jumpToEndButton->addAction(jumpToEndAction);
	jumpToEndButton->setEnabled(false);
	QObject::connect(jumpToEndButton, &QPushButton::clicked,
			jumpToEndAction, &QAction::trigger);

	playbackControlLayout = new QGridLayout(playbackControlWidget);
	playbackControlLayout->addWidget(timeLabel, 0, 0);
	playbackControlLayout->addWidget(timeLine, 0, 1, 1, 2);
	playbackControlLayout->addWidget(totalTimeLabel, 0, 3);
	playbackControlLayout->addWidget(totalTimeLine, 0, 4, 1, 2);
	playbackControlLayout->addWidget(jumpToStartButton, 1, 0);
	playbackControlLayout->addWidget(jumpBackwardButton, 1, 1);
	playbackControlLayout->addWidget(startPlaybackButton, 1, 2, 1, 2);
	playbackControlLayout->addWidget(jumpForwardButton, 1, 4);
	playbackControlLayout->addWidget(jumpToEndButton, 1, 5);

	playbackControlWidget->setLayout(playbackControlLayout);
	playbackControlDockWidget->setFloating(false);
	playbackControlDockWidget->setWidget(playbackControlWidget);
	addDockWidget(Qt::TopDockWidgetArea, playbackControlDockWidget);

}

void MeaviewWindow::initDisplaySettingsWidget() 
{
	dataConfigurationLabel = new QLabel("Channel view:", displaySettingsWidget);
	dataConfigurationLabel->setAlignment(Qt::AlignRight);
	dataConfigurationBox = new QComboBox(displaySettingsWidget);
	dataConfigurationBox->setToolTip("Set arrangement of subplots to match an electrode configuration");
	dataConfigurationBox->setEnabled(false);

	refreshIntervalLabel = new QLabel("Refresh:", displaySettingsWidget);
	refreshIntervalLabel->setAlignment(Qt::AlignRight);
	refreshIntervalBox = new QSpinBox(displaySettingsWidget);
	refreshIntervalBox->setSingleStep(plotwindow::RefreshStepSize);
	refreshIntervalBox->setRange(plotwindow::MinRefreshInterval, plotwindow::MaxRefreshInterval);
	refreshIntervalBox->setSuffix(" ms");
	refreshIntervalBox->setValue(plotwindow::DefaultRefreshInterval);
	refreshIntervalBox->setToolTip("Interval at which data plots refresh");
	QObject::connect(refreshIntervalBox, 
			static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			this, &MeaviewWindow::updateRefresh);

	scaleLabel = new QLabel("Scale:", displaySettingsWidget);
	scaleLabel->setAlignment(Qt::AlignRight);
	scaleBox = new QDoubleSpinBox(displaySettingsWidget);
	scaleBox->setToolTip("Set the y-axis scaling");
	scaleBox->setRange(0, 1000);
	scaleBox->setSingleStep(0.1);
	scaleBox->setSuffix(" V");
	scaleBox->setValue(plotwindow::McsDefaultDisplayRange);
	scaleBox->setDecimals(2);
	QObject::connect(scaleBox, 
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			this, &MeaviewWindow::updateScale);

	autoscaleBox = new QCheckBox("Autoscale", displaySettingsWidget);
	autoscaleBox->setToolTip("If checked, each subplot is scaled to fit its data");
	autoscaleBox->setTristate(false);
	autoscaleBox->setChecked(false);
	QObject::connect(autoscaleBox, &QCheckBox::stateChanged,
			this, &MeaviewWindow::updateAutoscale);

	displaySettingsLayout = new QGridLayout(displaySettingsWidget);
	displaySettingsLayout->addWidget(dataConfigurationLabel, 0, 0);
	displaySettingsLayout->addWidget(dataConfigurationBox, 0, 1);
	displaySettingsLayout->addWidget(refreshIntervalLabel, 0, 2);
	displaySettingsLayout->addWidget(refreshIntervalBox, 0, 3);
	displaySettingsLayout->addWidget(scaleLabel, 1, 0);
	displaySettingsLayout->addWidget(scaleBox, 1, 1);
	displaySettingsLayout->addWidget(autoscaleBox, 1, 2);

	displaySettingsWidget->setLayout(displaySettingsLayout);
	displaySettingsDockWidget->setFloating(false);
	displaySettingsDockWidget->setWidget(displaySettingsWidget);
	addDockWidget(Qt::TopDockWidgetArea, displaySettingsDockWidget);
	tabifyDockWidget(displaySettingsDockWidget, playbackControlDockWidget);
}

void MeaviewWindow::initPlotWindow() 
{
	plotWindow = new plotwindow::PlotWindow(this);
	setCentralWidget(plotWindow);
	QObject::connect(plotWindow, &plotwindow::PlotWindow::plotRefreshed,
			this, &MeaviewWindow::updateTime);
	QObject::connect(minifyAction, &QAction::triggered,
			plotWindow, &plotwindow::PlotWindow::minify);
	QObject::connect(showInspectorsAction, &QAction::triggered,
			plotWindow, &plotwindow::PlotWindow::toggleInspectorsVisible);
	QObject::connect(plotWindow, &plotwindow::PlotWindow::numInspectorsChanged,
			this, &MeaviewWindow::updateInspectorAction);
}

void MeaviewWindow::initSignals() 
{
	QObject::connect(this, &MeaviewWindow::recordingFinished,
			this, &MeaviewWindow::endRecording);
	QObject::connect(requestTimer, &QTimer::timeout,
			this, &MeaviewWindow::requestData);
}

void MeaviewWindow::requestData()
{
	if (!client)
		return;
	auto tmp = settings.value("display/refresh").toDouble() / 1000.;
	qDebug() << "Client requesting" << lastDataReceived << "to" << 
		lastDataReceived + tmp;
	client->getData(lastDataReceived, lastDataReceived + tmp);
}

void MeaviewWindow::connectToDataServer() 
{
	client.clear();
	client = new BldsClient(serverLine->text());
	QObject::connect(client, &BldsClient::connected,
			this, &MeaviewWindow::handleServerConnection);
	serverLine->setEnabled(false);
	connectToDataServerAction->setEnabled(false);
	connectToDataServerButton->setText("Cancel");
	QObject::disconnect(connectToDataServerButton, &QPushButton::clicked,
			connectToDataServerAction, &QAction::triggered);
	QObject::connect(connectToDataServerButton, &QPushButton::clicked,
			this, &MeaviewWindow::cancelDataServerConnectionAttempt);
	statusBar()->showMessage("Connecting to data server...");
	client->connect();
}

void MeaviewWindow::cancelDataServerConnectionAttempt() 
{
	serverLine->setEnabled(true);
	connectToDataServerAction->setEnabled(true);
	connectToDataServerButton->setText("Connect");
	QObject::disconnect(connectToDataServerButton, &QPushButton::clicked,
			this, &MeaviewWindow::cancelDataServerConnectionAttempt);
	QObject::connect(connectToDataServerButton, &QPushButton::clicked,
			connectToDataServerAction, &QAction::triggered);
	QObject::disconnect(client, &BldsClient::connected,
			this, &MeaviewWindow::handleServerConnection);
	client->deleteLater();
	client.clear();
	statusBar()->showMessage("Connection to data server canceled", StatusMessageTimeout);
}

void MeaviewWindow::handleServerConnection(bool made) 
{
	if (made) {
		/* Get the status of the server. */
		QObject::connect(client, &BldsClient::serverStatus,
				this, &MeaviewWindow::handleInitialServerStatusReply);
		client->requestServerStatus();

		/*
		auto deviceType = dataClient->get(target::type).toLower();
		settings.setValue("data/array", deviceType);
		settings.setValue("data/sample-rate", dataClient->get(target::sample_rate));
		QString tmp;
		if (deviceType.startsWith("hidens")) {
			settings.setValue("display/scale-multiplier", 1e-6);
			scaleBox->setSuffix(" uV");
			scaleBox->setValue(plotwindow::HiDensDefaultDisplayRange);
			scaleBox->setMaximum(plotwindow::HiDensMaxDisplayRange);
			scaleBox->setSingleStep(10);
			scaleBox->setDecimals(0);
			tmp = "HiDens";

		} else {
			settings.setValue("display/scale-multiplier", 1.0);
			scaleBox->setSuffix(" V");
			scaleBox->setValue(plotwindow::McsDefaultDisplayRange);
			scaleBox->setMaximum(plotwindow::McsMaxDisplayRange);
			scaleBox->setDecimals(2);
			scaleBox->setSingleStep(0.1);
			tmp = "MCS";

		}
		*/
		statusBar()->showMessage("Connected to Baccus lab data server",
				StatusMessageTimeout);

		/* Set up the UI to enable disconnecting from the server. */
		connectToDataServerButton->setEnabled(true);
		connectToDataServerButton->setText("Disconnect");
		disconnectFromDataServerAction->setEnabled(true);
		QObject::disconnect(connectToDataServerButton, 0, 0, 0);
		QObject::connect(connectToDataServerButton, &QPushButton::clicked,
				disconnectFromDataServerAction, &QAction::trigger);
		serverLine->setEnabled(false);

	} else {

		serverLine->setEnabled(true);
		connectToDataServerAction->setEnabled(true);
		disconnectFromDataServerAction->setEnabled(false);
		connectToDataServerButton->setText("Connect");
		QObject::disconnect(connectToDataServerButton, &QPushButton::clicked,
				this, &MeaviewWindow::cancelDataServerConnectionAttempt);
		QObject::connect(connectToDataServerButton, &QPushButton::clicked,
				connectToDataServerAction, &QAction::triggered);
		client->deleteLater();
		client.clear();
		statusBar()->showMessage("Error connecting to data server.",
				StatusMessageTimeout);
		QMessageBox::warning(this, "Connection error",
				"Could not connect to the data server. Please verify that the server is"
				" running, the entered IP address or hostname is correct, and that the"
				" MEA device itself is powered.");
	}
}

void MeaviewWindow::handleInitialServerStatusReply(const QJsonObject& status)
{
	/* Determine the most basic information about the server and source. */
	auto exists = status["source-exists"].toBool();
	settings.setValue("source/exists", exists);
	settings.setValue("recording/exists", status["recording-exists"].toBool());
	settings.setValue("recording/length", status["recording-length"].toInt());
	settings.setValue("recording/position", status["recording-position"].toDouble());

	if (exists) {
		QObject::connect(client, &BldsClient::sourceStatus,
				this, &MeaviewWindow::handleInitialSourceStatusReply);
		client->requestSourceStatus();
	} else {
		// not sure
	}

}

void MeaviewWindow::handleInitialSourceStatusReply(bool exists, const QJsonObject& status)
{
	if (exists) {
		auto type = status["source-type"].toString();
		auto array = status["device-type"].toString();
		settings.setValue("data/array", array);
		auto nchannels = status["nchannels"].toInt();
		settings.setValue("data/nchannels", nchannels);
		plotWindow->setupWindow(array, nchannels);

		startPlaybackButton->setEnabled(true);
		startPlaybackAction->setEnabled(true);
		QObject::connect(startPlaybackAction, &QAction::triggered,
				this, &MeaviewWindow::startPlayback);

		QObject::connect(client, &BldsClient::data,
				this, &MeaviewWindow::receiveDataFrame);

	} else {
		// not sure
	}
}

void MeaviewWindow::handleServerError(QString msg) 
{
	QMessageBox::critical(this, "Connection interrupted",
			QString("Connection to the data server has been interrupted. %1").arg(msg));
	disconnectFromDataServer();
}

void MeaviewWindow::disconnectFromDataServer() 
{
	if (!client)
		return;

	disconnectFromDataServerAction->setEnabled(false);
	connectToDataServerAction->setEnabled(true);
	connectToDataServerButton->setText("Connect");
	QObject::disconnect(connectToDataServerButton, &QPushButton::clicked,
			disconnectFromDataServerAction, &QAction::trigger);
	QObject::connect(connectToDataServerButton, &QPushButton::clicked,
			connectToDataServerAction, &QAction::trigger);

	serverLine->setEnabled(true);
	settings.remove("data/hidens-configuration");

	QObject::disconnect(dataConfigurationBox, 0, 0, 0);
	dataConfigurationBox->clear();
	dataConfigurationBox->setEnabled(false);
	setPlaybackMovementButtonsEnabled(false);

	QObject::disconnect(client, 0, 0, 0);
	client->disconnect();
	client->deleteLater();
	client.clear();

	plotWindow->clear();

	statusBar()->showMessage("Disconnected from data server", StatusMessageTimeout);
}

void MeaviewWindow::setPlaybackMovementButtonsEnabled(bool enabled) 
{
	jumpToStartButton->setEnabled(enabled);
	jumpToStartAction->setEnabled(enabled);
	jumpBackwardButton->setEnabled(enabled);
	jumpBackwardAction->setEnabled(enabled);
	jumpForwardButton->setEnabled(enabled);
	jumpForwardAction->setEnabled(enabled);
	jumpToEndButton->setEnabled(enabled);
	jumpToEndAction->setEnabled(enabled);
}

void MeaviewWindow::showHidensConfiguration() 
{
	if (!settings.value("data/array").toString().startsWith("hidens"))
		return;
	auto win = new configwindow::ConfigWindow(hidensConfiguration);
	win->show();
}

void MeaviewWindow::pausePlayback() 
{
	// need to actually stop requesting data via client
	requestTimer->stop();

	statusBar()->showMessage("Vizualization paused", StatusMessageTimeout);
	playbackStatus = PlaybackStatus::Paused;
	
	setPlaybackMovementButtonsEnabled(true);
	startPlaybackButton->setText("Start");
	startPlaybackAction->setText(tr("&Start"));
	QObject::disconnect(startPlaybackAction, &QAction::triggered,
			this, &MeaviewWindow::pausePlayback);
	QObject::connect(startPlaybackAction, &QAction::triggered,
			this, &MeaviewWindow::startPlayback);
}

void MeaviewWindow::startPlayback() 
{
	requestTimer->start();

	statusBar()->showMessage("Visualization started", StatusMessageTimeout);
	playbackStatus = PlaybackStatus::Playing;

	setPlaybackMovementButtonsEnabled(false);
	startPlaybackButton->setText("Pause");
	startPlaybackAction->setText(tr("&Pause"));
	QObject::disconnect(startPlaybackAction, &QAction::triggered,
			this, &MeaviewWindow::startPlayback);
	QObject::connect(startPlaybackAction, &QAction::triggered,
			this, &MeaviewWindow::pausePlayback);
}

void MeaviewWindow::endRecording() 
{
	if (client) {
		client->disconnect(); // just disconnect, don't fuck with others
	}
	requestTimer->stop();
	//stopPlayback();
	lastDataPlotted = 0.;
	lastDataPlotted = 0.;

	statusBar()->showMessage("Recording ended", StatusMessageTimeout * 2);
}

void MeaviewWindow::receiveDataFrame(const DataFrame& frame)
{
	plotWindow->transferDataToSubplots(frame);
	lastDataReceived = frame.stop();
	lastDataPlotted = frame.stop();
}

void MeaviewWindow::updateTime()
{
	timeLine->setText(QString("%1 - %2").arg(
				lastDataPlotted - settings.value("display/refresh").toDouble(), 
				0, 'f', 1).arg(lastDataPlotted, 0, 'f', 1));
}

void MeaviewWindow::jumpToStart() 
{
	lastDataPlotted = 0.;
	client->getData(lastDataPlotted, 
			lastDataPlotted + settings.value("display/refresh").toDouble());
}

void MeaviewWindow::jumpBackward() 
{
	auto diff = settings.value("display/refresh").toDouble();
	if (lastDataPlotted > diff) {
		lastDataPlotted = qMax(0.0, lastDataPlotted - 2 * diff);
		client->getData(lastDataPlotted, lastDataPlotted + diff);
	}
}

void MeaviewWindow::jumpForward() 
{
	auto diff = settings.value("display/refresh").toDouble();
	if ((lastDataReceived - lastDataPlotted) > diff)
		client->getData(lastDataPlotted, lastDataPlotted + diff);
}

void MeaviewWindow::jumpToEnd() 
{
	auto diff = settings.value("display/refresh").toDouble();
	lastDataPlotted = lastDataReceived;
	client->getData(lastDataPlotted, lastDataPlotted + diff);
}

void MeaviewWindow::updateAutoscale(int state) 
{
	scaleBox->setEnabled(state != Qt::Checked);
	settings.setValue("display/autoscale", state == Qt::Checked);
}

void MeaviewWindow::updateScale(double scale) 
{
	settings.setValue("display/scale", scale);
}

void MeaviewWindow::updateRefresh(int refresh) 
{
	requestTimer->setInterval(refresh);
	settings.setValue("display/refresh", refresh);
}

void MeaviewWindow::minify(bool checked) 
{
	showServerDockWidget->setVisible(!checked);
	showDisplaySettingsDockWidget->setVisible(!checked);
	showPlaybackControlDockWidget->setVisible(!checked);
}

void MeaviewWindow::updateInspectorAction(int n) 
{
	showInspectorsAction->setEnabled(n > 0);
	showInspectorsAction->setChecked(n > 0);
}

void MeaviewWindow::storeHidensConfiguration() 
{
	QVariantList config;
	config.reserve(hidensConfiguration.size());
	for (auto& el : hidensConfiguration) {
		config.append(QPoint{ 
				static_cast<int>(el.xpos), 
				static_cast<int>(el.ypos) 
			});
	}
	settings.setValue("data/hidens-configuration", config);
}

}; // end meaviewwindow namespace
}; // end meaview namespace

