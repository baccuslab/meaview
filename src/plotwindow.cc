/*! \file plotwindow.cc
 *
 * Implementation of main window for displaying array data.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#include "plotwindow.h"

#include <QFont>

#include <cmath>
#include <cstring>

namespace meaview {
namespace plotwindow {

PlotWindow::PlotWindow(QWidget* parent) 
	: QWidget(parent, Qt::Widget)
{
	setGeometry(meaviewwindow::WindowPosition.first, meaviewwindow::WindowPosition.second, 
			meaviewwindow::WindowSize.first, meaviewwindow::WindowSize.second);
	initThreadPool();
	initPlot();
	QObject::connect(plot, &QCustomPlot::mouseDoubleClick,
			this, &PlotWindow::createChannelInspector);
	QObject::connect(plot, &QCustomPlot::mousePress,
			this, &PlotWindow::handleChannelClick);
	fullPosition = geometry();
	show();
	lower();
	qRegisterMetaType<QList<subplot::Subplot*> >("QList<subplot::Subplot*>");
}

PlotWindow::~PlotWindow()
{
	replotWorker->deleteLater();
	replotThread->quit();
	replotThread->wait();
	replotThread->deleteLater();
	for (auto& worker : workers)
		worker->deleteLater();
	for (auto& thread : threads) {
		thread->quit();
		thread->wait();
		thread->deleteLater();
	}
	clear();
}

void PlotWindow::closeEvent(QCloseEvent* ev)
{
	if (ev->spontaneous()) {
		ev->ignore();
		emit hideRequest();
	} else
		ev->accept();
}

void PlotWindow::initThreadPool()
{
	replotWorker = new plotworker::PlotWorker;
	replotThread = new QThread;
	replotWorker->moveToThread(replotThread);
	QObject::connect(this, &PlotWindow::allSubplotsUpdated,
			replotWorker, &plotworker::PlotWorker::replot);
	QObject::connect(replotWorker, &plotworker::PlotWorker::plotUpdated,
			this, &PlotWindow::handlePlotUpdated);
	replotThread->start();
	
	for (auto i = 0; i < qMax(nthreads - 1, 1); i++) {
		threads.append(new QThread());
		workers.append(new plotworker::PlotWorker);
		workers.at(i)->moveToThread(threads.at(i));
		QObject::connect(this, &PlotWindow::sendDataToPlotWorker,
				workers.at(i), &plotworker::PlotWorker::transferDataToSubplot);
		QObject::connect(this, &PlotWindow::clearSubplots,
				workers.at(i), &plotworker::PlotWorker::clearSubplots);
		threads.at(i)->start();
	}
}

void PlotWindow::initPlot()
{
	/* Create plot */
	plot = new QCustomPlot(this);
	plot->plotLayout()->removeAt(0);
	plot->plotLayout()->setRowSpacing(plotwindow::RowSpacing);
	plot->plotLayout()->setColumnSpacing(plotwindow::ColumnSpacing);
	plot->setBackground(plotwindow::BackgroundColor);

	/* Set layout */
	layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(plot);
	setLayout(layout);
}

void PlotWindow::setupWindow(const QString& array, const int nchannels)
{
	clear();
	nsubplots = nchannels;
	computePlotColors();
	subplotsUpdated.resize(nsubplots);
	subplotsUpdated.fill(false);
	setWindowTitle(QString("MeaView: Data (%1)").arg(array));

	/* Setup plot grid and view */
	computePlotGridSize();
	createPlotGrid();
	createChannelView();

	int worker = 0;
	bool isHidens = settings.value("data/array").toString().startsWith("hidens");
	QString label;
	for (auto i = 0; i < gridSize.first; i++) {
		for (auto j = 0; j < gridSize.second; j++) {
			auto idx = i * gridSize.second + j;
			if (idx >= nchannels)
				break;

			/* Create a subplot for this channel */
			auto position = view.at(idx);
			int chan = position.first * gridSize.second + position.second;
			if (isHidens) {
				if (chan == (nsubplots - 1))
					label = plotwindow::HidensPhotodiodeName;
				else
					label = QString::number(chan);
			} else {
				if (plotwindow::McsChannelNames.contains(chan))
					label = plotwindow::McsChannelNames[chan];
				else
					label = QString::number(chan);
			}
			auto sp = new subplot::Subplot(chan, label, idx, position, plot);
			QObject::connect(sp, &subplot::Subplot::plotReady, 
					this, &PlotWindow::incrementNumPlotsUpdated);
			plot->plotLayout()->addElement(position.first, position.second, sp->rect());

			/* Assign this subplot to a worker */
			workers.at(worker)->addSubplot(sp);
			subplots.append(sp);
			worker += 1;
			worker %= workers.size();
		}
	}
	plot->replot();
}

void PlotWindow::blockResize(const bool block)
{
	resizeBlocked = block;
	if (block)
		setFixedSize(size());
	else
		setFixedSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void PlotWindow::incrementNumPlotsUpdated(const int idx)
{
	subplotsUpdated.setBit(idx);
	if (subplotsUpdated.count(true) < subplotsUpdated.size())
		return;
	blockResize(true);
	emit allSubplotsUpdated(plot, &lock, subplots);
}

void PlotWindow::handlePlotUpdated(const int npoints)
{
	blockResize(false);
	subplotsUpdated.fill(false);
	emit plotRefreshed(npoints);
}

void PlotWindow::toggleVisible()
{
	setVisible(!isVisible());
}

void PlotWindow::toggleInspectorsVisible()
{
	for (auto& each : inspectors)
		each->setVisible(!each->isVisible());
}

void PlotWindow::clearAllData()
{
	lock.lockForWrite();
	for (auto& sp : subplots)
		sp->clearData();
	clickedPlots.clear();
	plot->replot();
	lock.unlock();
}

void PlotWindow::clear()
{
	while (!inspectors.isEmpty()) {
		auto each = inspectors.takeFirst();
		QObject::disconnect(plot, &QCustomPlot::afterReplot, each, 0);
		each->deleteLater();
	}
	emit numInspectorsChanged(inspectors.size());
	lock.lockForWrite();
	emit clearSubplots();
	while (!subplots.isEmpty())
		subplots.takeFirst()->deleteLater();
	plot->clearGraphs();
	plot->plotLayout()->clear();
	plot->replot();
	lock.unlock();
	setWindowTitle("MeaView: Data");
}

void PlotWindow::createChannelInspector(QMouseEvent* event)
{
	auto sp = findSubplotContainingPoint(event->pos());
	if (!sp)
		return;

	for (auto& inspector: inspectors) {
		if (sp->channel() == inspector->channel()) {
			inspector->activateWindow();
			inspector->raise();
			return;
		}
	}

	lock.lockForRead();
	auto c = new channelinspector::ChannelInspector(plot,
			sp->graph(), sp->channel(), sp->label(), this);
	lock.unlock();
	QObject::connect(c, &channelinspector::ChannelInspector::aboutToClose,
			this, &PlotWindow::removeChannelInspector);
	inspectors.append(c);
	if (inspectors.size() > 1) {
		auto pos = inspectors.at(inspectors.size() - 2)->pos();
		c->move(pos.x() + channelinspector::WindowSpacing.first, 
				pos.y() + channelinspector::WindowSpacing.second);
	}
	c->show();
	emit numInspectorsChanged(inspectors.size());
}

void PlotWindow::removeChannelInspector(int channel)
{
	if (inspectors.size() == 0)
		return;

	for (auto i = 0; i < inspectors.size(); i++) {
		if (inspectors.at(i)->channel() == channel) {
			delete inspectors.takeAt(i);
			emit numInspectorsChanged(inspectors.size());
			return;
		}
	}
}

void PlotWindow::handleChannelClick(QMouseEvent* event)
{
	if (event->button() != Qt::RightButton)
		return;
	auto sp = findSubplotContainingPoint(event->pos());
	if (!sp)
		return;
	if (clickedPlots.contains(sp))
		clickedPlots.remove(sp);
	else
		clickedPlots.insert(sp);
}

subplot::Subplot* PlotWindow::findSubplotContainingPoint(const QPoint& point)
{
	auto rects = plot->axisRects();
	for (auto& subplot : subplots) {
		if (subplot->rect()->outerRect().contains(point))
			return subplot;
	}
	return nullptr;
}

void PlotWindow::transferDataToSubplots(const DataFrame& frame)
{
	auto data = frame.data();
	transferDataToSubplots(data);
}

void PlotWindow::transferDataToSubplots(const DataFrame::Samples& d)
{
	for (auto c = 0; c < nsubplots; c++) {
		auto chan = subplots.at(c)->channel();
		QVector<double> vec(d.n_rows);
		//std::memcpy(vec.data(), d.colptr(chan), sizeof(double) * d.n_rows); 
		for (auto i = 0; i < vec.size(); i++)
			vec[i] = d(i, chan);
		emit sendDataToPlotWorker(subplots.at(c), vec, &lock,
				clickedPlots.contains(subplots.at(c)));
	}
}

void PlotWindow::computePlotGridSize()
{
	if (settings.value("data/array").toString().startsWith("hidens")) {
		gridSize.first = std::ceil(std::sqrt(nsubplots));
		gridSize.second = std::ceil(double(nsubplots) / gridSize.first);
	} else
		gridSize = plotwindow::McsChannelViewSizeMap[
			settings.value("display/view").toString()];
}

void PlotWindow::createPlotGrid()
{
	/* Remove any extant plot elements */
	if (plot->plotLayout()->elementCount() > 0) {
		for (auto i = 0; i < nsubplots; i++)
			plot->plotLayout()->takeAt(view.at(i).first * gridSize.second 
					+ view.at(i).second);
	}
	plot->plotLayout()->expandTo(0, 0);
	plot->plotLayout()->simplify();
	plot->plotLayout()->expandTo(gridSize.first, gridSize.second);
}

void PlotWindow::createChannelView()
{
	view.clear();
	if (settings.value("data/array").toString().startsWith("hidens")) {

		/* Retrieve electrode positions */
		auto varList = settings.value("data/hidens-configuration").toList();
		QList<Electrode> config;
		for (auto& var : varList) {
			auto point = var.toPoint();
			config.append(Electrode{0, static_cast<uint32_t>(point.x()), 
					0, static_cast<uint32_t>(point.y()), 0, 0});
		}

		/* Argsort electrode positions */
		std::vector<int> idx(config.size());
		std::iota(idx.begin(), idx.end(), 0);

		/* First find "minimal" electrode, that closest to origin */
		std::nth_element(idx.begin(), idx.begin(), idx.end(), 
				[&config](int i, int j)->bool {
					return configwindow::ElectrodeSorter(config.at(i), config.at(j));
			});

		/* Sort by distance to this base electrode */
		auto sorter = [&config](int i, int j)->bool {
			return configwindow::ElectrodeSorterDist(config.at(0),
					config.at(i), config.at(j));
		};
		std::sort(idx.begin(), idx.end(), sorter);

		/* Construct unsorted view */
		decltype(view) unsortedView;
		for (auto i = 0; i < gridSize.first; i++) {
			for (auto j = 0; j < gridSize.second; j++) {
				auto ix = i * gridSize.second + j;
				if ( ix >= nsubplots )
					break;
				unsortedView << QPair<int, int>(i, j);
			}
		}

		/* Sort view based on electrode positions */
		view.reserve(nsubplots);
		for (auto i = 0; i < config.size(); i++) {
			view.insert(i, unsortedView.at(idx[i]));
		}

		/* Add position for photodiode at the last subplot */
		view.insert(nsubplots - 1, unsortedView.at(nsubplots - 1));

	} else {
		view = plotwindow::McsChannelViewMap[
			settings.value("display/view").toString()];
	}
}

void PlotWindow::updateChannelView()
{
	computePlotGridSize();
	createPlotGrid();
	createChannelView();
	moveSubplots();
	updateInspectorSources();
}

void PlotWindow::updateInspectorSources()
{
	if (inspectors.isEmpty())
		return;
	for (auto& inspector : inspectors) {
		subplot::Subplot* source = nullptr;
		for (auto& sp : subplots) {
			if (sp->channel() == inspector->channel()) {
				source = sp;
				break;
			}
		}
		inspector->updateSourceGraph(source->graph());
	}
}

void PlotWindow::moveSubplots()
{
	for (auto i = 0; i < nsubplots; i++) {
		auto newPos = view.at(i);
		subplots[i]->setPosition(newPos);
		plot->plotLayout()->addElement(newPos.first, newPos.second, 
				subplots[i]->rect());
	}
}

const plotwindow::ChannelView& PlotWindow::currentView() const
{
	return view;
}

void PlotWindow::minify(bool min)
{
	auto blocked = resizeBlocked;
	blockResize(false);
	if (min) {
		fullPosition = geometry();
		lock.lockForWrite();
		setGeometry(
				meaviewwindow::MinimalWindowPosition.first, 
				meaviewwindow::MinimalWindowPosition.second,
				meaviewwindow::MinimalWindowSize.first,
				meaviewwindow::MinimalWindowSize.second
			);
		lock.unlock();
		stackInspectors();
	} else {
		lock.lockForWrite();
		setGeometry(fullPosition);
		show();
		lock.unlock();
		unstackInspectors();
	}
	blockResize(blocked);
}

void PlotWindow::stackInspectors()
{
	int ypos = ((y() + frameGeometry().height()) +
			1.5 * channelinspector::WindowSpacing.first);
	for (auto& inspector : inspectors) {
		inspector->saveFullPosition();
		inspector->setGeometry(
				x(), ypos, 
				channelinspector::MinimalWindowSize.first,
				channelinspector::MinimalWindowSize.second
			);
		ypos += channelinspector::MinimalWindowSize.second + 
			channelinspector::WindowSpacing.first / 2;
	}
}

void PlotWindow::unstackInspectors()
{
	for (auto& inspector : inspectors) {
		inspector->setGeometry(inspector->fullPosition());
		inspector->raise();
	}
}

void PlotWindow::computePlotColors()
{
	QVariantList pens;
	pens.reserve(nsubplots);
	int spacing = static_cast<int>(360. / nsubplots);
	for (auto i = 0; i < nsubplots; i++) {
		pens << QPen{QColor::fromHsv(i * spacing, 
				PlotPenSaturation, PlotPenValue)};
	}
	settings.setValue("display/plot-pens", pens);
}

}; // end plotwindow namespace
}; // end meaview namespace
