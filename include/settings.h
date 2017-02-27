/*! \file settings.h
 *
 * Single header file for application-wide settings
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#ifndef _MEAVIEW_SETTINGS_H_
#define _MEAVIEW_SETTINGS_H_

#include <cmath> 	// for std::{pow, sqrt}

#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QPair>
#include <QPen>
#include <QColor>
#include <QSet>
#include <QFont>
#include <QDir>

#include "configuration.h" // To define electrode sorter for HiDens configurations

namespace meaview {

/*! Default sample rate for MCS data */
const double McsSampleRate = 10000;

/*! Default sample rate for HiDens data */
const double HiDensSampleRate = 20000;

namespace meaviewwindow {

	/*! The window position, upper left corner */
	const QPair<int, int> WindowPosition = { 
#ifdef Q_OS_WIN
			50, 50 
#else
			0, 0
#endif
	};

	/*! The window width and height */
	const QPair<int, int> WindowSize = { 1200, 1000 };

	/*! Window position when the application is in its "minimal" state.
	 * See the user guide for details.
	 */
	const QPair<int, int> MinimalWindowPosition = { 
#ifdef Q_OS_WIN
			50, 50 
#else
			0, 0
#endif
	};

	/*! Window size when application is in "minimal" state.
	 * See the user guide for details.
	 */
	const QPair<int, int> MinimalWindowSize = { 500, 500 };

	/*! Timeout for status bar messages (milliseconds) */
	const int StatusMessageTimeout = 10000;

	/*! Default IP address for data server */
	const QString DefaultServerHost = "localhost";

	/*! Size of data chunks to request, in *milliseconds*. */
	const int DataChunkRequestSize = 100;

}; // end meaviewwindow namespace

namespace plotwindow {

	/*! Default plot refresh interval in seconds. */
	const double DefaultRefreshInterval = 2.0;

	/*! Minimum plot refresh interval in seconds */
	const double MinRefreshInterval = 0.5;

	/*! Maximum plot refresh interval in seconds */
	const double MaxRefreshInterval = 10.0;

	/*! Step size for refresh intervals in seconds */
	const double RefreshStepSize = 0.5;

	/*! Default scale for MCS data plots, in volts */
	const float McsDefaultDisplayRange = { 0.5 }; 
	const float McsMaxDisplayRange = { 10 };

	/*! Default scale for HiDens data plots, in volts.
	 * Comes from range of 5V, 8-bit data, and a usual pre-ADC (on-chip)
	 * amplifier gain of about 900. So least-significant bit is
	 * ~ 5 / (1<<8) / 900 or about 1e-5V. This is in microvolts.
	 */
	const float HiDensDefaultDisplayRange = { 100. };
	const float HiDensMaxDisplayRange = { 10000. };

	/*! Channel used to show photodiode information */
	const int McsPhotodiodeChannel = 0;
	
	/*! Channel carrying intracellular voltage data */
	const int McsIntracellularVoltageChannel = 1;
	
	/*! Channel carrying intracellular current data */
	const int McsIntracellularCurrentChannel = 2;

	/*! Unused MCS data channel */
	const int McsUnusedChannel = 3;

	/*! MCS channels that are always autoscaled */
	const QList<int> McsAutoscaledChannels = {
		McsPhotodiodeChannel,
		McsIntracellularVoltageChannel,
		McsIntracellularCurrentChannel,
		McsUnusedChannel
	};

	/*! MCS special channel names */
	const QMap<int, QString> McsChannelNames = {
		{0, "Photodiode"}, 
		{1, "Intracellular Vm"},
	    {2, "Intracellular I"},
		{3, "Unused"},
	};

	/*! HiDens has no special channel names, but the last
	 * channel is always the photodiode.
	 */
	const QString HidensPhotodiodeName = "Photodiode";

	/*! Size of plotting pen (points) */
	const double PlotPenSize = 1.0;

	/*! HSV saturation used for lines showing data */
	const int PlotPenSaturation = 100;

	/*! HSV value used for lines showing data */
	const int PlotPenValue = 100;

	/*! Pen used to plot data in selected plots */
	const QPen SelectedPlotPen = QPen(QColor(225, 225, 225));

	/*! Background color for plot. */
	const QBrush BackgroundColor { QColor{10, 10, 10} };

	/*! Spacing between subplot rows */
	const int RowSpacing = -20;
	
	/*! Spacing between subplot columns */
	const int ColumnSpacing = -20;

	/*! Possible channel arrangement views */
	const QStringList McsChannelViewStrings = {
		"Channel order",
		"Standard",
		"Hexagonal"
	};

	/*! Default channel view */
	const QString DefaultChannelView = "Channel order";

	/*! Channel order view definition */
	using ChannelView = QList<QPair<int, int> >;
	const ChannelView McsChannelOrderView {
		{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7},
		{1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7},
		{2, 0}, {2, 1}, {2, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7},
		{3, 0}, {3, 1}, {3, 2}, {3, 3}, {3, 4}, {3, 5}, {3, 6}, {3, 7},
		{4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6}, {4, 7},
		{5, 0}, {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 5}, {5, 6}, {5, 7},
		{6, 0}, {6, 1}, {6, 2}, {6, 3}, {6, 4}, {6, 5}, {6, 6}, {6, 7},
		{7, 0}, {7, 1}, {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6}, {7, 7},
	};

	/*! Layout of low-density (standard) array */
	const ChannelView McsStandardView {
		{0, 0}, {0, 7}, {7, 0}, {7, 7}, // photodiode, intraVM, intraCur, extra
		{6, 3}, {7, 3}, {5, 3}, {4, 3}, {7, 2}, {6, 2}, {7, 1}, {5, 2},
		{6, 1}, {6, 0}, {5, 1}, {5, 0}, {4, 2}, {4, 1}, {4, 0}, {3, 0},
		{3, 1}, {3, 2}, {2, 0}, {2, 1}, {1, 0}, {1, 1}, {2, 2}, {0, 1},
		{1, 2}, {0, 2}, {3, 3}, {2, 3}, {0, 3}, {1, 3}, {1, 4}, {0, 4}, 
		{2, 4}, {3, 4}, {0, 5}, {1, 5}, {0, 6}, {2, 5}, {1, 6}, {1, 7},
		{2, 6}, {2, 7}, {3, 5}, {3, 6}, {3, 7}, {4, 7}, {4, 6}, {4, 5},
		{5, 7}, {5, 6}, {6, 7}, {6, 6}, {5, 5}, {7, 6}, {6, 5}, {7, 5},
		{4, 4}, {5, 4}, {7, 4}, {6, 4}
	};

	/*! Layout of hexagonal array */
	const ChannelView McsHexagonalView {
		{0, 0}, {0, 7}, {8, 0}, {8, 7}, 
		{7, 3}, {8, 2}, {6, 3}, {7, 2}, {8, 1}, {4, 2}, {7, 1}, {6, 2}, 
		{5, 2}, {6, 1}, {5, 1}, {5, 0}, {4, 1}, {4, 0}, {8, 6}, {3, 3}, // {8, 7} is ref electrode
		{3, 0}, {3, 1}, {3, 2}, {2, 1}, {2, 2}, {1, 1}, {2, 3}, {0, 1}, 
		{3, 4}, {0, 2}, {1, 3}, {2, 4}, {0, 3}, {0, 4}, {1, 4}, {2, 5},
		{1, 5}, {0, 5}, {4, 3}, {1, 6}, {2, 6}, {3, 5}, {2, 7}, {3, 6},
		{4, 4}, {3, 7}, {1, 2}, {4, 5}, {4, 6}, {5, 4}, {5, 7}, {6, 7},
		{5, 6}, {6, 6}, {5, 5}, {7, 6}, {6, 5}, {7, 5}, {8, 5}, {5, 3}, 
		{8, 4}, {7, 4}, {6, 4}, {8, 3}
	};

	/*! Mapping from view names to views themselves */
	const QMap<QString, ChannelView> McsChannelViewMap = {
		{"Channel order", McsChannelOrderView}, 
		{"Standard", McsStandardView},
		{"Hexagonal", McsHexagonalView}
	};

	/*! Mapping between channel views and their sizes (rows and columns) */
	const QMap<QString, QPair<int, int> > McsChannelViewSizeMap = {
		{"Channel order", {8, 8}}, 
		{"Standard", {8, 8}},
		{"Hexagonal", {9, 8}}
	};

	/*! Allowed channels views for HiDens array system */
	const QStringList HidensChannelViewStrings = {
		"Channel order", 
	};

}; // end plotwindow namespace

namespace subplot {
	/*! Size of label fonts */
	const int FontSize = 10;

	/*! Font used to label subplots with the channel number */
	const QFont LabelFont("Helvetica", FontSize, QFont::Light);

	/*! Space between label and subplot itself */
	const int LabelPadding = 2;

	/* Color of lines and labels */
	const QColor LabelColor { 255, 255, 255 };

}; // end subplot namespace

namespace channelinspector {

	/*! Size of a new channel inspector window */
	const QPair<int, int> WindowSize = { 600, 250 };

	/*! Spacing between successive inspector windows */
	const QPair<int, int> WindowSpacing = { 50, 50 };

	/*! Size of the window when application is minified */
	const QPair<int, int> MinimalWindowSize = {
			meaviewwindow::MinimalWindowSize.first, 75
		};

	/* Background color for inspector plots */
	const QBrush BackgroundColor { QColor{10, 10, 10} };

	/*! Color of lines and labels */
	const QColor LabelColor { 255, 255, 255 };

}; // end channelinspector namespace

namespace configwindow {

	/*! Size of a new window */
	const QPair<int, int> WindowSize = { 2, 250 };

	/*! X-axis range, in microns */
	const QPair<float, float> XAxisRange = { 0, 2.0 };

	/*! Y-axis range, in microns */
	const QPair<float, float> YAxisRange = { 0, 2.5 };

	/*! Length of ticks */
	const int TickLength = 1;

	/*! Size of points */
	const int PointSize = 10;

	/*! Function used to sort electrodes by position in a HiDens configuration.
	 * This is used to color data by location, so that nearby electrodes have
	 * similar colors.
	 *
	 * Electrodes are sorted by x-position, then y-position.
	 */
	const auto ElectrodeSorter = [](Electrode e1, Electrode e2) -> bool {
		if (e1.xpos != e2.xpos) {
			return (e1.xpos < e2.xpos);
		} else {
			return (e1.ypos <= e2.ypos);
		}
	};

	/*! Function used to sort electrodes by position in a HiDens configuration.
	 * This is used to color data by location, so that nearby electrodes have
	 * similar colors.
	 *
	 * Electrodes are sorted by distance from a given base electrode.
	 */
	const auto ElectrodeSorterDist = [](Electrode base, Electrode e1, Electrode e2) -> bool {
		auto dist = [](const Electrode& el1, const Electrode& el2) -> double {
				return std::sqrt(std::pow(el2.xpos, 2) - std::pow(el1.xpos, 2) + 
				std::pow(el2.ypos, 2) - std::pow(el1.ypos, 2));
			};
		return dist(base, e1) < dist(base, e2);
	};


}; // end configwindow namespace

}; // end meaview namespace

#endif

