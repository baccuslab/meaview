/*! \file plotworker.cc
 *
 * Implementation of PlotWorker class.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#include "plotworker.h"

namespace meaview {
namespace plotworker {

PlotWorker::PlotWorker(QObject* parent)
	: QObject(parent)
{
	autoscale = settings.value("display/autoscale").toBool();
	qRegisterMetaType<QVector<double> >("QVector<double>");
}

PlotWorker::~PlotWorker()
{
}

void PlotWorker::transferDataToSubplot(subplot::Subplot* subplot, 
		QVector<double> data, QReadWriteLock* lock, const bool clicked)
{
	if (!subplots.contains(subplot))
		return;
	subplot->addDataToBackBuffer(data, lock, clicked);
}

void PlotWorker::constructXData(int n)
{
	if (xdata.size() == n)
		return;
	xdata.resize(n);
	std::iota(xdata.begin(), xdata.end(), 0.0);
}

void PlotWorker::replot(QCustomPlot* plot, QReadWriteLock* lock, 
		QList<subplot::Subplot*> sps)
{
	lock->lockForWrite();
	plot->replot();
	lock->unlock();
	emit plotUpdated((*sps.begin())->graph()->data()->size());
}

void PlotWorker::addSubplot(subplot::Subplot* sp)
{
	subplots.insert(sp);
}

void PlotWorker::clearSubplots()
{
	subplots.clear();
}

}; // end plotworker namespace
}; // end meaview namespace

