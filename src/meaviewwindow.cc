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
	position(0.)
{
	setWindowTitle("meaview");
	setGeometry(WindowPosition.first, WindowPosition.second,
			WindowSize.first, WindowSize.second);

	/* Initialize all sub-widgets. */
	createDockWidgets();
	initSettings();
	initMenus();
	initServerWidget();
	initPlaybackControlWidget();
	initDisplaySettingsWidget();
	initPlotWindow();

	/* Connect any initial signals and slots. */
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
	settings.setValue("data/request-size", meaviewwindow::DataChunkRequestSize);
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

	/* Menu for controlling interaction with the BLDS. */
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

	/* Menu for controlling playback. */
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

	/* Menu for controlling view and windows. */
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

	showHidensConfigurationAction = new QAction(tr("Show &HiDens configuration"), 
			viewMenu);
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
	viewMenu->addAction(minifyAction);

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
	dataConfigurationBox->setToolTip(
			"Set arrangement of subplots to match an electrode configuration");
	dataConfigurationBox->setEnabled(false);

	refreshIntervalLabel = new QLabel("Refresh:", displaySettingsWidget);
	refreshIntervalLabel->setAlignment(Qt::AlignRight);
	refreshIntervalBox = new QDoubleSpinBox(displaySettingsWidget);
	refreshIntervalBox->setSingleStep(plotwindow::RefreshStepSize);
	refreshIntervalBox->setRange(plotwindow::MinRefreshInterval, 
			plotwindow::MaxRefreshInterval);
	refreshIntervalBox->setSuffix(" s");
	refreshIntervalBox->setValue(plotwindow::DefaultRefreshInterval);
	refreshIntervalBox->setToolTip("Interval at which data plots refresh");
	QObject::connect(refreshIntervalBox, 
			static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
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
}

void MeaviewWindow::requestData()
{
	if (!client)
		return;
	client->getData(position, position + settings.value("display/refresh").toDouble());
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
	position = settings.value("recording/position").toDouble();

	if (exists) {

		/* Request the source status.
		 *
		 * If this is a Hidens array, first request the configuration 
		 * to determine how to initialize the plot window.
		 */
		QObject::connect(client, &BldsClient::sourceStatus,
				this, &MeaviewWindow::handleInitialSourceStatusReply);
		if (status["device-type"].toString().startsWith("hidens")) {
			connections.insert("configuration", 
					QObject::connect(client, &BldsClient::getSourceResponse,
					[&](const QString& param, bool /* valid */, 
							const QVariant& data) -> void {
						if (param != "configuration")
							return;
						if (connections.contains("configuration"))
							QObject::disconnect(connections.take("configuration"));
						hidensConfiguration = data.value<QConfiguration>();
						storeHidensConfiguration();
						client->requestSourceStatus();
					}));
			client->getSource("configuration");
		} else {
			client->requestSourceStatus();
		}

	} else {
		QMessageBox::warning(this, "No data source", "There is no active data source "
				"managed by the BLDS at this time. Connect again after the source has "
				"been created.");
		settings.remove("source/exists");
		settings.remove("recording/exists");
		settings.remove("recording/length");
		settings.remove("recording/position");
		position = 0.0;
		disconnectFromDataServer();
	}

}

void MeaviewWindow::handleInitialSourceStatusReply(bool exists, 
		const QJsonObject& status)
{
	if (exists) {
		auto type = status["source-type"].toString();
		auto array = status["device-type"].toString();
		auto nchannels = status["nchannels"].toInt();
		settings.setValue("data/array", array);
		settings.setValue("data/nchannels", nchannels);
		settings.setValue("data/gain", status["gain"].toDouble());
		settings.setValue("data/sample-rate", status["sample-rate"].toDouble());

		initChannelViewMenu();
		plotWindow->setupWindow(array, nchannels);

		if (array.startsWith("hidens")) {
			settings.setValue("display/scale-multiplier", 1e-6);
			scaleBox->setSuffix(" uV");
			scaleBox->setValue(plotwindow::HiDensDefaultDisplayRange);
			scaleBox->setMaximum(plotwindow::HiDensMaxDisplayRange);
			scaleBox->setSingleStep(10);
			scaleBox->setDecimals(0);
			showHidensConfigurationAction->setEnabled(true);
		} else {
			settings.setValue("display/scale-multiplier", 1.0);
			scaleBox->setSuffix(" V");
			scaleBox->setValue(plotwindow::McsDefaultDisplayRange);
			scaleBox->setMaximum(plotwindow::McsMaxDisplayRange);
			scaleBox->setDecimals(2);
			scaleBox->setSingleStep(0.1);
		}

		startPlaybackButton->setEnabled(true);
		startPlaybackAction->setEnabled(true);
		QObject::connect(startPlaybackAction, &QAction::triggered,
				this, &MeaviewWindow::startPlayback);

		QObject::connect(client, &BldsClient::data,
				this, &MeaviewWindow::receiveDataFrame);
		QObject::connect(client, &BldsClient::error,
				this, &MeaviewWindow::handleServerError);

	} else {
		// not sure. If not recording, probably just don't enable the start button
		// and check periodically for the recording existing/running. Enable once
		// it is or clear window when the source is deleted.
	}
}

void MeaviewWindow::handleServerError(QString msg) 
{
	QMessageBox::critical(this, "Server error",
			QString("An error was received from the server:\n\n%1").arg(msg));
	endRecording();
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
	showHidensConfigurationAction->setEnabled(false);
	timeLine->setText("");

	QObject::disconnect(dataConfigurationBox, 0, 0, 0);
	dataConfigurationBox->clear();
	dataConfigurationBox->setEnabled(false);
	setPlaybackMovementButtonsEnabled(false);

	QObject::disconnect(client, 0, 0, 0);
	client->disconnect();
	client->deleteLater();
	client.clear();

	plotWindow->clear();
	position = 0.0;

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
	auto win = new configwindow::ConfigWindow(hidensConfiguration.toStdVector());
	win->show();
}

void MeaviewWindow::pausePlayback() 
{
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
	client->get("recording-position");
	connections.insert("get-position",
			QObject::connect(client, &BldsClient::getResponse,
				[&](QString param, bool /* valid */, QVariant value) -> void {
					if (param != "recording-position")
						return;
					QObject::disconnect(connections.take("get-position"));
					position = value.toFloat();
					requestData();
				}));

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
	playbackStatus = PlaybackStatus::Paused;

	setPlaybackMovementButtonsEnabled(false);
	startPlaybackButton->setText("Start");
	QObject::disconnect(startPlaybackAction, &QAction::triggered,
			this, 0);
	QObject::connect(startPlaybackAction, &QAction::triggered,
			this, &MeaviewWindow::startPlayback);
	startPlaybackButton->setEnabled(false);
	totalTimeLine->setText("0");
	position = 0.;

	QObject::disconnect(dataConfigurationBox, 0, 0, 0);
	dataConfigurationBox->clear();
	dataConfigurationBox->setEnabled(false);

	statusBar()->showMessage("Recording ended", StatusMessageTimeout * 2);
}

void MeaviewWindow::receiveDataFrame(const DataFrame& frame)
{
	plotWindow->transferDataToSubplots(frame.data());
	position = frame.stop();
	if (playbackStatus == PlaybackStatus::Playing)
		requestData();
}

void MeaviewWindow::updateTime()
{
	timeLine->setText(QString("%1 - %2").arg(
				position - settings.value("display/refresh").toDouble(), 
				0, 'f', 1).arg(position, 0, 'f', 1));
}

void MeaviewWindow::jumpToStart() 
{
	position = 0.;
	client->getData(position, position + settings.value("display/refresh").toDouble());
}

void MeaviewWindow::jumpBackward() 
{
	auto refresh = settings.value("display/refresh").toDouble();
	if (position > refresh) {
		position = qMax(0.0, position - 2 * refresh);
		client->getData(position, position + refresh);
	}
}

void MeaviewWindow::jumpForward() 
{
	auto refresh = settings.value("display/refresh").toDouble();
	client->getData(position, position + refresh);
}

void MeaviewWindow::jumpToEnd() 
{
	connections.insert("position", 
			QObject::connect(client, &BldsClient::getResponse,
			[&](const QString& param, bool /* valid */, const QVariant& value) -> void {
				if (param != "recording-position")
					return;
				if (connections.contains("position"))
					QObject::disconnect(connections.take("position"));
				position = value.toDouble() - 
					settings.value("display/refresh").toDouble();
				client->getData(position, position + 
						settings.value("display/refresh").toDouble());
			}));
	client->get("recording-position");
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

void MeaviewWindow::updateRefresh(double refresh) 
{
	settings.setValue("display/refresh", refresh);
}

void MeaviewWindow::minify(bool checked) 
{
	if (checked) {
		windowPosition = geometry();
		setGeometry(windowPosition.x(), windowPosition.y(),
				MinimalWindowSize.first, MinimalWindowSize.second);
	} else {
		setGeometry(windowPosition);
	}

	auto visible = !checked;
	showServerDockWidget->setVisible(visible);
	serverDockWidget->setVisible(visible);
	showDisplaySettingsDockWidget->setVisible(visible);
	displaySettingsDockWidget->setVisible(visible);
	showPlaybackControlDockWidget->setVisible(visible);
	playbackControlDockWidget->setVisible(visible);
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
		/*
		 * Note: Have to use QList::push_back, because QList::append
		 * chooses the overload that appends all items in the list,
		 * rather than the list itself.
		 */
		config.push_back(QVariantList{ 
				static_cast<int>(el.index),
				static_cast<int>(el.xpos), 
				static_cast<int>(el.x), 
				static_cast<int>(el.ypos),
				static_cast<int>(el.y),
			});
	}
	settings.setValue("data/hidens-configuration", config);
}

void MeaviewWindow::initChannelViewMenu()
{
	if (settings.value("data/array").toString().startsWith("hidens")) {
		dataConfigurationBox->addItems(plotwindow::HidensChannelViewStrings);
	} else {
		dataConfigurationBox->addItems(plotwindow::McsChannelViewStrings);
	}
	QObject::connect(dataConfigurationBox, &QComboBox::currentTextChanged,
			this, &MeaviewWindow::updateChannelView);
	dataConfigurationBox->setEnabled(true);
}

void MeaviewWindow::updateChannelView()
{
	settings.setValue("display/view", dataConfigurationBox->currentText());
	plotWindow->updateChannelView();
}

}; // end meaviewwindow namespace
}; // end meaview namespace

