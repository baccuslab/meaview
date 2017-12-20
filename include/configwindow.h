/*! \file configwindow.h
 *
 * Simple window class for showing a given HiDens electrode 
 * array configuration.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#ifndef _MEAVIEW_CONFIG_WINDOW_H_
#define _MEAVIEW_CONFIG_WINDOW_H_

#include "settings.h"
#include "qcustomplot.h"

#include "configuration.h"

#include <QtWidgets>
#include <QtCore>

#include <cmath>

namespace meaview {

/*! \namespace configwindow
 *
 * The configwindow namespace contains classes and data related
 * to the window used to show a Hidens configuration.
 */
namespace configwindow {

/*! \class ConfigWindow
 *
 * The ConfigWindow class is a simple widget which plots the 
 * current configuration of the Hidens chip. It displays the
 * extent of the chip and a colored dot for each connected
 * electrode, where the color corresponds to the color of the
 * data plotted in the grid of subplots. (These should be arranged
 * roughly as a function of their distance from the origin of
 * the chip (0, 0).)
 */
class ConfigWindow : public QWidget {

	Q_OBJECT

	public:
		/*! Construct a ConfigWindow.
		 *
		 * \param config The configuration to be displayed.
		 */
		ConfigWindow(const Configuration& config, 
				QWidget* parent = 0);

		/*! Destruct a ConfigWindow. */
		~ConfigWindow();

	private slots:

		/*! Reset the extent of the axes, to show the full Hidens chip. */
		void resetAxes();

		/*! Display the current position of the mouse as a tooltip,
		 * when the user hovers over the chip.
		 */
		void showPosition(QMouseEvent* event);

		/*! Update the title of the ConfigWindow to show the
		 * (x, y) position and channel number of a clicked electrode.
		 */
		void labelPoint(QCPAbstractPlottable* p, QMouseEvent* event);

	private:

		/*! Actually plot the current configuration. */
		void plotConfiguration();

		/*! Helper function used to sort the electrodes.
		 * 
		 * \param x The x position of the origin.
		 * \param y The y position of the origin.
		 * \param el The electrode whose distance to the origin is returned.
		 *
		 * This should sort electrodes by their Euclidean distance from
		 * the given position.
		 */
		inline double distance(double x, double y, const Electrode& el) const
		{
			return std::sqrt(std::pow(el.xpos - x, 2) + std::pow(el.ypos - y, 2));
		}

		/*! Application-wide settings. */
		QSettings settings;

		/*! The configuration plotted in this window. */
		Configuration config;

		/*! Plot object which displays the configuration data. */
		QCustomPlot* plot;

		/*! The layout of this window. */
		QGridLayout* layout;

}; // end ConfigWindow class
}; // end configwindow namespace
}; // end meaview namespace

#endif

