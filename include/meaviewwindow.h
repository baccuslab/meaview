/*! \file meaviewwindow.h
 *
 * Main window for the meaview application
 *
 * (C) 2016 Benjamin Naecker bnaecker@stanford.edu
 */

#ifndef MEAVIEW_WINDOW_H_
#define MEAVIEW_WINDOW_H_

#include "settings.h"
#include "plotwindow.h"

#include "configuration.h" // from libdata-source/include, for QConfiguration

#include "blds-client.h" // from libblds-client/include, for BldsClient

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*! \namespace meaview
 *
 * The meaview namespace is the top-level namespace for all components
 * it contains all subnamespaces and classes for the different components,
 * as well as all settings for each class.
 */
namespace meaview {

/*! \namespace meaviewwindow
 *
 * Namespace related to the MeaviewWindow class.
 */
namespace meaviewwindow {

/*! Simple enum class for specifying whether data is currently
 * being played or not.
 */
enum class PlaybackStatus {
	Playing,
	Paused
};

/*! \class MeaviewWindow
 *
 * The MeaviewWindow class is the main window in the meaview application.
 * It provides a small widget for connecting to the BLDS from which data
 * is streamed, and for controlling the playback of data, such as the
 * refresh rate.
 *
 * The main portion of the window is a grid of subplots, which display 
 * the data in realtime streamed from the BLDS application. Users are
 * able to start and stop this stream of data as they wish, and to
 * jump around in time to view any data that has already been collected.
 */
class MeaviewWindow : public QMainWindow {
	Q_OBJECT

	public:
		/*! Construct a MeaviewWindow */
		MeaviewWindow(QWidget* parent = nullptr);

		/*! Destroy a MeaviewWindow. */
		~MeaviewWindow();

	signals:
		/*! This signal is emitted when data stops streaming
		 * from the BLDS because the recording has finished or
		 * been otherwise stopped by another remote client.
		 */
		void recordingFinished();

	private slots:

		/*! This slot is called when the user clicks the "Connect"
		 * button, and attempts to establish a connection with 
		 * the BLDS.
		 */
		void connectToDataServer();

		/*! This slot is called to handle the result of an attempted
		 * connection to the BLDS.
		 */
		void handleServerConnection(bool made);

		/*! This slot is called to cancel a pending connection to
		 * the BLDS.
		 */
		void cancelDataServerConnectionAttempt();

		/*! Handle an initial request to the server for its status. */
		void handleInitialServerStatusReply(const QJsonObject& status);
		
		/*! Handle a request to the server for the status of the source. */
		void handleInitialSourceStatusReply(bool exists, const QJsonObject& status);

		/*! This slot is called to handle an error that occurs
		 * when communicating with the BLDS.
		 */
		void handleServerError(QString msg);

		/*! This slot disconnects from the BLDS. */
		void disconnectFromDataServer();

		/*! This slot enables the interface related to jumping
		 * around a recording.
		 */
		void setPlaybackMovementButtonsEnabled(bool enabled);

		/*! This slot opens a window which shows the current 
		 * configuration of a Hidens chip, if available.
		 */
		void showHidensConfiguration();

		/*! This slot starts the playing of data, if available. */
		void startPlayback();

		/*! This slop pauses the playing of data. */
		void pausePlayback();

		/*! Used to handle end of a recording. */
		void endRecording();

		/*! This slot is used to update the line showing the 
		 * current position in the recording.
		 */
		void updateTime();

		/*! This slot handles jumping playback to the very initial
		 * portion of the recording, if possible.
		 */
		void jumpToStart();

		/*! This slot handles jumping playback back by one refresh
		 * interval, if possible.
		 */
		void jumpBackward();

		/*! This slot handles jumping playback forward by one
		 * refresh interval, if possible.
		 */
		void jumpForward();

		/*! This slot handles jumping playback to the most recent
		 * data, if possible.
		 */
		void jumpToEnd();

		/*! This slot is called to update whether each plot is 
		 * automatically scaled to fit its data range.
		 */
		void updateAutoscale(int state);

		/*! This slot updates the scale factor for each subplot, used
		 * to scale the y-axes to some fixed range.
		 */
		void updateScale(double scale);

		/*! This slot updates the refresh interval of each plot. */
		void updateRefresh(double refresh);

		void requestData();

		void receiveDataFrame(const DataFrame& frame);

		/*! This slot minifies the window, making it small but visible.
		 * This can be useful for keeping an eye on the display without it
		 * taking over a screen.
		 */
		void minify(bool checked);

		/*! This slot handles updating any open channel inspector
		 * plots.
		 */
		void updateInspectorAction(int numInspectors);

	private:

		/* Create the dock widgets for interacting with the server and
		 * manipulating playback.
		 */
		void createDockWidgets();

		/* Initialize the global settings from the values in settings.h
		 * and anything inside meaview.conf.
		 */
		void initSettings();

		/* Construct and initialize the menus used for controlling 
		 * the interface.
		 */
		void initMenus();

		/* Initialize the widget used to connect to the server. */
		void initServerWidget();

		/* Initialize the widget for controlling playback. */
		void initPlaybackControlWidget();

		/* Initialize the widget used for manipulating the settings
		 * of the display.
		 */
		void initDisplaySettingsWidget();

		/* Initialize the main plot window showing data. */
		void initPlotWindow();

		/* Connect all initial signals and slots. */
		void initSignals();

		/* Read settings parameters from meaview.conf. */
		void readConfigurationFile();

		/* Return the number of samples that should be plotted
		 * during each refresh interval.
		 */
		int numSamplesPerPlotBlock();

		/*! Store the current Hidens configuration inside the 
		 * settings object, so they can be read by other 
		 * classes.
		 */
		void storeHidensConfiguration();

		/* Current status of playback. */
		PlaybackStatus playbackStatus;

		/* Main settings object, which stores all information shared
		 * across threads and objects about the data, playback, and
		 * display parameters.
		 */
		QSettings settings;

		/* Main client object used to communicate with the BLDS
		 * and to get data from it.
		 */
		QPointer<BldsClient> client;

		/* Main window showing all subplots of data. This is the
		 * central widget of the MeaviewWindow class.
		 */
		QPointer<plotwindow::PlotWindow> plotWindow;

		/* The current hidens configuration, if any. */
		QConfiguration hidensConfiguration;

		/* Current position in the recording. This specifies both
		 * the last sample plotted and the last sample received.
		 */
		double position;

		/* The main menu bar, with all sub-menus. */
		QMenuBar* menuBar;

		/* The menu for interacting with the BLDS. */
		QMenu* serverMenu;

		/* The menu for manipulating playback. */
		QMenu* playbackMenu;

		/* The menu for controlling windows and views of data. */
		QMenu* viewMenu;
	
		/* Action for connecting to the data server. */
		QAction* connectToDataServerAction;

		/* Action for disconnection from the data server. */
		QAction* disconnectFromDataServerAction;

		/* Action for starting the playback of data. */
		QAction* startPlaybackAction;

		/* Action for pausing the playback of data. */
		QAction* pausePlaybackAction;

		/* Action for triggering jumping backward by one refresh interval. */
		QAction* jumpBackwardAction;

		/* Action for triggering jumping forward by one refresh interval. */
		QAction* jumpForwardAction;

		/* Action triggering jumping to the start of the recording. */
		QAction* jumpToStartAction;

		/* Action triggering jumping to the end of the recording. */
		QAction* jumpToEndAction;

		/* Action for showing/hiding the server dock widget. */
		QAction* showServerDockWidget;

		/* Action for showing/hiding the playback control dock widget. */
		QAction* showPlaybackControlDockWidget;

		/* Action for showing/hiding the display setttings dock widget. */
		QAction* showDisplaySettingsDockWidget;

		/* Action for showing/hiding any open channel inspectors. */
		QAction* showInspectorsAction;

		/* Action for showing/hiding the hidens configuration window. */
		QAction* showHidensConfigurationAction;

		/* Action for showing/hiding the analog output for this recording,
		 * if any.
		 */
		QAction* showAnalogOutputAction;

		/* Action to minify the main window. */
		QAction* minifyAction;

		/* Main layout manager. */
		QGridLayout* mainLayout;

		/* Dock widget containing elements related to the BLDS. */
		QDockWidget* serverDockWidget;

		/* Widget living inside the server dock widget. */
		QWidget* serverWidget;

		/* Layout manager for the server widget. */
		QGridLayout* serverLayout;

		/* Labels the line showing the hostname. */
		QLabel* serverLabel;

		/* Line showing the BLDS hostname. */
		QLineEdit* serverLine;

		/* Button for connecting to/disconnecting from the BLDS. */
		QPushButton* connectToDataServerButton;

		/* Dock widget containing playback controls. */
		QDockWidget* playbackControlDockWidget;

		/* Widget for the playback control dock widget. */
		QWidget* playbackControlWidget;

		/* Layout manager for the playback controls. */
		QGridLayout* playbackControlLayout;

		/* Button to start/stop the playback. */
		QPushButton* startPlaybackButton;

		/* Button to jump to the beginning of the data stream. */
		QPushButton* jumpToStartButton;

		/* Button to jump to the end of the data stream (most recent data). */
		QPushButton* jumpToEndButton;

		/* Button to jump back one refresh interval. */
		QPushButton* jumpBackwardButton;

		/* Button to jump forward one refresh interval. */
		QPushButton* jumpForwardButton;

		/* Labels the field showing the current time in the recording. */
		QLabel* timeLabel;

		/* Line showing the current time in the recording. */
		QLineEdit* timeLine;
		
		/* Labels the line showing full recording length. */
		QLabel* totalTimeLabel;

		/* Line showing the full length of the recording. */
		QLineEdit* totalTimeLine;

		/* Dock widget for the display settings. */
		QDockWidget* displaySettingsDockWidget;

		/* Widget for the settings dock. */
		QWidget* displaySettingsWidget;

		/* Layout manager for the display settings. */
		QGridLayout* displaySettingsLayout;

		/* Labels the current refresh interval. */
		QLabel* refreshIntervalLabel;

		/* Spin box displaying and used to change the refresh interval
		 * of the plots.
		 */
		QDoubleSpinBox* refreshIntervalBox;

		/* Labels the combo box used to select a new configuration
		 * of the subplots.
		 */
		QLabel* dataConfigurationLabel;

		/* Combo box used to select the actual configuration of the 
		 * subplot boxes in the window.
		 */
		QComboBox* dataConfigurationBox;

		/* Labels the field showing the plot y-scale. */
		QLabel* scaleLabel;

		/* Box for showing and changing the y-scale. */
		QDoubleSpinBox* scaleBox;

		/* Check box used to select whether the plots autoscale. */
		QCheckBox* autoscaleBox;

		/* Stored connections to make it easier to connect/disconnect
		 * callbacks in various places.
		 */
		QMap<QString, QMetaObject::Connection> connections;
};

}; // end meaviewwindow namespace
}; // end meaview namespace

#endif

