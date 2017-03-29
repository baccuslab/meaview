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
	setGeometry(meaviewwindow::WindowPosition.first, 
			meaviewwindow::WindowPosition.second, 
			meaviewwindow::WindowSize.first, meaviewwindow::WindowSize.second);
	initThreadPool();
	initPlot();
	QObject::connect(plot, &QCustomPlot::mouseDoubleClick,
			this, &PlotWindow::createChannelInspector);
	QObject::connect(plot, &QCustomPlot::mousePress,
			this, &PlotWindow::handleChannelClick);
	show();
	lower();
}

PlotWindow::~PlotWindow()
{
	emit deleteSubplots();
	for (auto& thread : transferThreads) {
		thread->quit();
		thread->wait();
		thread->deleteLater();
	}
}

void PlotWindow::initThreadPool()
{
	for (auto i = 0; i < qMax(nthreads - 1, 1); i++) {
		transferThreads.append(new QThread());
		transferThreads.at(i)->start();
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
	nsubplots = nchannels;
	computePlotColors();
	subplotsUpdated.resize(nsubplots);
	subplotsUpdated.fill(false);
	subplotsDeleted.resize(nsubplots);
	subplotsDeleted.fill(false);

	/* Setup plot grid and view */
	computePlotGridSize();
	createPlotGrid();
	createChannelView();

	int threadNum = 0;
	bool isHidens = array.startsWith("hidens");
	QString label;
	for (auto i = 0; i < gridSize.first; i++) {
		for (auto j = 0; j < gridSize.second; j++) {
			auto idx = i * gridSize.second + j;
			if (idx >= nchannels)
				break;

			/* Compute position and channel number of this new
			 * subplot in the current grid.
			 */
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

			/* Create a subplot for this channel */
			auto sp = new subplot::Subplot(chan, label, idx, position, plot);

			/* Connect signals/slots for communicating with subplot */
			QObject::connect(this, &PlotWindow::sendDataToSubplot,
					sp, &subplot::Subplot::handleNewData);
			QObject::connect(sp, &subplot::Subplot::plotReady, 
					this, &PlotWindow::incrementNumPlotsUpdated);
			QObject::connect(this, &PlotWindow::deleteSubplots,
					sp, &subplot::Subplot::requestDelete);
			QObject::connect(sp, &subplot::Subplot::deleted,
					this, &PlotWindow::handleSubplotDeleted);
			plot->plotLayout()->addElement(position.first, 
					position.second, sp->rect());

			/* Move this subplot to the appropriate background thread. */
			subplots.append(sp);
			sp->moveToThread(transferThreads.at(threadNum));
			threadNum += 1;
			threadNum %= transferThreads.size();
		}
	}
	plot->replot();
}

void PlotWindow::incrementNumPlotsUpdated(const int idx)
{
	subplotsUpdated.setBit(idx);
	if (subplotsUpdated.count(true) < subplotsUpdated.size())
		return;
	replot();
}

void PlotWindow::handleSubplotDeleted(int index)
{
	subplotsDeleted.setBit(index);
	if (subplotsDeleted.count(true) < subplotsDeleted.size())
		return;
	handleAllSubplotsDeleted();
}

void PlotWindow::toggleInspectorsVisible()
{
	for (auto& each : inspectors)
		each->setVisible(!each->isVisible());
}

void PlotWindow::clear()
{
	while (!inspectors.isEmpty()) {
		auto each = inspectors.takeFirst();
		QObject::disconnect(plot, &QCustomPlot::afterReplot, each, 0);
		each->deleteLater();
	}
	emit numInspectorsChanged(inspectors.size());
	emit deleteSubplots();
}

void PlotWindow::handleAllSubplotsDeleted()
{
	lock.lockForWrite();
	subplots.clear();
	plot->plotLayout()->clear();
	plot->clearGraphs();
	plot->replot();
	lock.unlock();
	subplotsDeleted.fill(false);
	emit cleared();
}

void PlotWindow::createChannelInspector(QMouseEvent* event)
{
	/* Find subplot for this click. */
	auto sp = findSubplotContainingPoint(event->pos());
	if (!sp)
		return;

	/* If an inspector already exists for this channel,
	 * just raise it.
	 */
	for (auto& inspector: inspectors) {
		if (sp->channel() == inspector->channel()) {
			inspector->activateWindow();
			inspector->raise();
			return;
		}
	}

	/* Create a new inspector from this channel. */
	lock.lockForRead();
	auto c = new channelinspector::ChannelInspector(plot,
			sp->graph(), sp->channel(), sp->label(), this);
	lock.unlock();
	QObject::connect(c, &channelinspector::ChannelInspector::aboutToClose,
			this, &PlotWindow::removeChannelInspector);

	/* Add to list and position it appropriately. */
	inspectors.append(c);
	if (inspectors.size() > 1) {
		auto pos = inspectors.at(inspectors.size() - 2)->pos();
		c->move(pos.x() + channelinspector::WindowSpacing.first, 
				pos.y() + channelinspector::WindowSpacing.second);
	}
	c->show();

	/* Notify. */
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

void PlotWindow::transferDataToSubplots(const DataFrame::Samples& d)
{
	for (auto c = 0; c < nsubplots; c++) {

		/* Copy appropriate channel into new vector.
		 *
		 * NOTE: The subplot will delete this, it *must not* be deleted
		 * in this method.
		 */
		auto chan = subplots.at(c)->channel();
		auto vec = new QVector<DataFrame::DataType>(d.n_rows);
		std::memcpy(vec->data(), d.colptr(chan),
				sizeof(DataFrame::DataType) * d.n_rows);

		/* Send data */
		emit sendDataToSubplot(subplots.at(c), vec, &lock, 
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
	plot->plotLayout()->clear();
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
	/* Compute size of new grid. */
	computePlotGridSize();

	/* Clear plot layout, without deleting subplots. */
	for (auto i = 0; i < nsubplots; i++) {
		if (plot->plotLayout()->elementAt(i)) {
			plot->plotLayout()->takeAt(i);
		}
	}
	plot->plotLayout()->simplify();

	/* Create new view and move subplots there. */
	createChannelView();
	moveSubplots();

	/* Update the source graph for any open channel inspectors. */
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
				qDebug() << source << sp;
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

		/* Compute new position. */
		auto newPos = view.at(i);
		subplots[i]->setPosition(newPos);

		/* Put into new position. */
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
	if (min) {
		stackInspectors();
	} else {
		unstackInspectors();
	}
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

void PlotWindow::replot()
{
	lock.lockForWrite();
	plot->replot();
	lock.unlock();
	subplotsUpdated.fill(false);
	emit plotRefreshed(subplots.first()->graph()->data()->size());
}

}; // end plotwindow namespace
}; // end meaview namespace

