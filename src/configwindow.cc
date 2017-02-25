/*! \file configwindow.cc
 *
 * Implementation of simple class to show HiDens configurations.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#include "configwindow.h"

#include <limits>
#include <algorithm> 	// nth_element, sort
#include <functional>	// bind

using namespace std::placeholders; // for _1, _2, etc

namespace meaview {
namespace configwindow {

ConfigWindow::ConfigWindow(const Configuration& conf, 
		QWidget* parent)
	: QWidget(parent, Qt::Window),
	config(conf)
{
	setWindowTitle("HiDens configuration");
	setGeometry(x(), y(), 
			configwindow::WindowSize.first, configwindow::WindowSize.second);

	/* Find the element closest to the origin, which will be the base element */
	std::nth_element(config.begin(), config.begin(), config.end(), configwindow::ElectrodeSorter);

	/* Use this as the base electrode, and sort the configuration by the distance
	 * to this electrode
	 */
	auto sorter = std::bind(configwindow::ElectrodeSorterDist, config[0], _1, _2);
	std::sort(config.begin(), config.end(), sorter);

	plot = new QCustomPlot(this);
	plot->setInteractions(QCP::iRangeDrag | 
			QCP::iRangeZoom | QCP::iSelectPlottables);
	plot->setBackground(plotwindow::BackgroundColor);
	plotConfiguration();
	resetAxes();
	plot->replot();

	layout = new QGridLayout(this);
	layout->addWidget(plot);
	setLayout(layout);
	show();

	QObject::connect(plot, &QCustomPlot::mouseDoubleClick,
			this, &ConfigWindow::resetAxes);
	QObject::connect(plot, &QCustomPlot::mouseMove,
			this, &ConfigWindow::showPosition);
	QObject::connect(plot, &QCustomPlot::plottableClick,
			this, &ConfigWindow::labelPoint);
}

ConfigWindow::~ConfigWindow()
{
	delete plot;
	delete layout;
}

void ConfigWindow::resetAxes() 
{
	plot->xAxis->setRange(configwindow::XAxisRange.first, 
			configwindow::XAxisRange.second);
	plot->yAxis->setRange(configwindow::YAxisRange.first, 
			configwindow::YAxisRange.second);
	plot->xAxis->setTickLength(0, configwindow::TickLength);
	plot->yAxis->setTickLength(0, configwindow::TickLength);
	plot->xAxis->setSubTickLengthIn(0);
	plot->xAxis->setTickLabelColor(subplot::LabelColor);
	plot->xAxis->setBasePen(subplot::LabelColor);
	plot->yAxis->setSubTickLengthIn(0);
	plot->xAxis->grid()->setVisible(false);
	plot->yAxis->grid()->setVisible(false);
	plot->xAxis->setTickLabelFont(subplot::LabelFont);
	plot->xAxis->setTickPen(subplot::LabelColor);
	plot->xAxis->setSubTickPen(subplot::LabelColor);
	plot->xAxis->setLabelFont(subplot::LabelFont);
	plot->xAxis->setLabelColor(subplot::LabelColor);
	plot->xAxis->setLabel("mm");
	plot->yAxis->setTickLabelFont(subplot::LabelFont);
	plot->yAxis->setTickLabelColor(subplot::LabelColor);
	plot->yAxis->setBasePen(subplot::LabelColor);
	plot->yAxis->setTickPen(subplot::LabelColor);
	plot->yAxis->setSubTickPen(subplot::LabelColor);
	plot->yAxis->setLabelFont(subplot::LabelFont);
	plot->yAxis->setLabelColor(subplot::LabelColor);
	plot->yAxis->setLabel("mm");
}

void ConfigWindow::plotConfiguration()
{
	auto pens = settings.value("display/plot-pens").toList();
	QCPDataMap data;
	for (decltype(config.size()) i = 0; i < config.size(); i++) {
		plot->addGraph();
		data.clear();
		data.insert(config.at(i).xpos, 
				QCPData{ config.at(i).xpos / 1e6, config.at(i).ypos / 1e6 });
		plot->graph(i)->setData(&data, true);
		plot->graph(i)->setLineStyle(QCPGraph::lsNone);
		plot->graph(i)->setScatterStyle(QCPScatterStyle(
					QCPScatterStyle::ssCircle, Qt::black,
					pens.at(i).value<QPen>().color(),
					configwindow::PointSize));
		plot->graph(i)->setSelectedPen(QPen(QBrush(Qt::red), 
					configwindow::PointSize * 2));
		plot->graph(i)->setSelectable(true);
		plot->graph(i)->valueAxis()->setNumberFormat("gb");
		plot->graph(i)->valueAxis()->setNumberPrecision(2);
	}
	plot->plotLayout()->insertRow(0);
	auto title = new QCPPlotTitle(plot, "Click electrode to view");
	title->setTextColor(subplot::LabelColor);
	plot->plotLayout()->addElement(0, 0, title);
}

void ConfigWindow::showPosition(QMouseEvent* event)
{
	double x = plot->xAxis->pixelToCoord(event->pos().x());
	double y = plot->yAxis->pixelToCoord(event->pos().y());
	setToolTip(QString("%1 mm, %2 mm").arg(x).arg(y));
}

void ConfigWindow::labelPoint(QCPAbstractPlottable* p, QMouseEvent* event)
{
	auto graph = dynamic_cast<QCPGraph*>(p);
	if (!graph)
		return;
	double x = plot->xAxis->pixelToCoord(event->pos().x());
	double y = plot->yAxis->pixelToCoord(event->pos().y());

	auto found = config.begin();
	auto dist = std::numeric_limits<double>::infinity();
	int idx = 0;
	for (decltype(config.size()) i = 0; i < config.size(); i++) {
		auto tmpDistance = distance(x * 1e6, y * 1e6, config.at(i));
		if (tmpDistance < dist) {
			*found = config.at(i);
			idx = i;
			dist = tmpDistance;
		}
	}
	auto title = dynamic_cast<QCPPlotTitle*>(plot->plotLayout()->elementAt(0));
	if (!title)
		return;
	title->setText(QString("Channel %1 (%2, %3)").arg(idx).arg(
				found->x).arg(found->y));
}

}; // end configwindow namespace

}; // end meaview namespace
