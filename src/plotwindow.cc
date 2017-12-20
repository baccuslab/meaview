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
	/* Request all subplots delete themselves and
	 * shutdown all transfer threads.
	 */
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

void PlotWindow::setupWindow(const QString& array, int nchannels)
{
	nsubplots = nchannels;
	subplotsUpdated.resize(nsubplots);
	subplotsUpdated.fill(false);
	subplotsDeleted.resize(nsubplots);
	subplotsDeleted.fill(false);

	/* Setup plot grid and view */
	computePlotGridSize();
	createPlotGrid();
	createChannelView();

	/* Compute the valid channels. */
	computePlotColors(computeValidDataChannels());

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
			QObject::connect(this, &PlotWindow::updateRefresh,
					sp, &subplot::Subplot::updatePlotBlockSize);
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

void PlotWindow::incrementNumPlotsUpdated(int idx, int npoints)
{
	/* Update our bitarray indicating that this plot 
	 * has been updated, and replot the whole grid if
	 * all have done so.
	 */
	subplotsUpdated.setBit(idx);
	if (subplotsUpdated.count(true) < subplotsUpdated.size())
		return;
	replot(npoints);
}

void PlotWindow::handleSubplotDeleted(int index)
{
	/* Update our bitarray indicating that this plot
	 * has been deleted, and clear the whole grid if
	 * all have been deleted.
	 */
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
	/* Delete all inspectors and request all subplots to delete. */
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
	/* Lock and clear all subplots/data/graphs/etc. */
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

	/* Find inspector and delete it. */
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

	/* Add clicked plot to set. The color is updated
	 * inside the `Subplot` class.
	 */
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

		/* Construct basic grid view. */
		view.clear();
		for (auto i = 0; i < gridSize.first; i++) {
			for (auto j = 0; j < gridSize.second; j++) {
				auto ix = i * gridSize.second + j;
				if ( ix >= nsubplots )
					break;
				view << QPair<int, int>(i, j);
			}
		}


	} else {
		view = plotwindow::McsChannelViewMap[
			settings.value("display/view").toString()];
	}
}

QMap<int, bool> PlotWindow::computeValidDataChannels()
{
	QMap<int, bool> valid;
	if (settings.value("data/array").toString().startsWith("hidens")) {
		/* Retrieve electrode positions */
		auto electrodes = settings.value("data/hidens-configuration").toList();
		for (auto i = 0; i < nsubplots; i++) {
			/* Invalid channels have 0 for their index. */
			valid.insert(i, electrodes.at(i).toList().at(0).toUInt() != 0);
		}
	} else {
		for (auto i = 0; i < nsubplots; i++) {
			valid.insert(i, true);
		}
	}
	return valid;
}

void PlotWindow::updateChannelView()
{
	/* Compute size of new grid. */
	computePlotGridSize();

	/* Clear plot layout, without deleting subplots. */
	for (auto i = 0; i < plot->plotLayout()->elementCount(); i++) {
		if (plot->plotLayout()->elementAt(i)) {
			plot->plotLayout()->takeAt(i);
		}
	}
	plot->plotLayout()->simplify();

	/* Create new view and move subplots there. */
	createChannelView();
	moveSubplots();
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
	/* Shrink all inspectors and stack them vertically. */
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

void PlotWindow::computePlotColors(const QMap<int, bool>& valid)
{
	/* Compute equally-spaced colors around the HSV space. */
	QVariantList pens;
	pens.reserve(nsubplots);
	int spacing = static_cast<int>(360. / nsubplots);
	for (auto i = 0; i < nsubplots; i++) {
		if (!valid.value(i)) {
			pens << QPen{InvalidPlotPenColor};
		} else {
			pens << QPen{QColor::fromHsv(i * spacing, 
					PlotPenSaturation, PlotPenValue)};
		}
	}
	settings.setValue("display/plot-pens", pens);
}

void PlotWindow::replot(int npoints)
{
	/* Thread-safe replotting. 
	 *
	 * A read-write lock is an unusual synchronization primitive, but
	 * it maps perfectly onto what is done here. The many transfer
	 * threads may be happily moving data to each subplot's back buffer,
	 * which happens with no synchronization. When they want to swap
	 * the front and back buffers, they must lock this for read. That's
	 * to allow multiple transfer workers to proceed, as they manage
	 * disjoint sets of subplots. This locks the lock for writing, meaning
	 * that none of those front-back swaps may happen while the plot
	 * is updating.
	 */
	lock.lockForWrite();
	plot->replot();
	lock.unlock();
	subplotsUpdated.fill(false);
	emit plotRefreshed(npoints);
}

}; // end plotwindow namespace
}; // end meaview namespace

