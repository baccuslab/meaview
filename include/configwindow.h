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
namespace configwindow {

class ConfigWindow : public QWidget {
	Q_OBJECT
	public:
		ConfigWindow(const Configuration& config, 
				QWidget* parent = 0);
		~ConfigWindow();

	private slots:
		void resetAxes();
		void showPosition(QMouseEvent* event);
		void labelPoint(QCPAbstractPlottable* p, QMouseEvent* event);

	private:
		void plotConfiguration();

		inline double distance(double x, double y, const Electrode& el) const
		{
			return std::sqrt(std::pow(el.xpos - x, 2) + std::pow(el.ypos - y, 2));
		}

		QSettings settings;
		Configuration config;
		QCustomPlot* plot;
		QGridLayout* layout;
		int nclicks = 0;

}; // end ConfigWindow class
}; // end configwindow namespace
}; // end meaview namespace

#endif

