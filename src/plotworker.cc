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
	m_autoscale = m_settings.value("display/autoscale").toBool();
	qRegisterMetaType<QVector<DataFrame::DataType> >("QVector<DataFrame::DataType>");
}

PlotWorker::~PlotWorker()
{
}

void PlotWorker::transferDataToSubplot(subplot::Subplot* subplot, 
		QVector<DataFrame::DataType> data, QReadWriteLock* lock, 
		const bool clicked)
{
	if (!m_subplots.contains(subplot))
		return;
	subplot->addDataToBackBuffer(data, lock, clicked);
}

void PlotWorker::constructXData(int n)
{
	if (m_xdata.size() == n)
		return;
	m_xdata.resize(n);
	std::iota(m_xdata.begin(), m_xdata.end(), 0.0);
}

void PlotWorker::addSubplot(subplot::Subplot* sp)
{
	m_subplots.insert(sp);
}

void PlotWorker::clearSubplots()
{
	m_subplots.clear();
}

}; // end plotworker namespace
}; // end meaview namespace

