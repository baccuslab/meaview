/*! \file channelinspector.cc
 *
 * Implementation of the ChannelInspector class.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#include "channelinspector.h"

namespace meaview {
namespace channelinspector {

ChannelInspector::ChannelInspector(QCustomPlot* parentPlot, 
		QCPGraph* source, int chan, const QString& label, QWidget* parent)
	: QWidget(parent, Qt::Window),
	ticks(3),
	tickLabels(3)
{
	channel_ = chan;
	sourceGraph = source;

	/* Create plot axis, graph, etc. */
	plot = new QCustomPlot(this);
	plot->setBackground(channelinspector::BackgroundColor);
	graph = plot->addGraph();
	graph->keyAxis()->setTicks(false);
	graph->keyAxis()->setTickLabels(false);
	graph->keyAxis()->grid()->setVisible(false);
	graph->keyAxis()->setRange(0, settings.value("sample-rate").toDouble() *
			settings.value("display/refresh").toDouble() / 1000);
	graph->keyAxis()->setBasePen(channelinspector::LabelColor);
	graph->valueAxis()->setTickLabelColor(channelinspector::LabelColor);
	graph->valueAxis()->grid()->setVisible(false);
	graph->valueAxis()->setBasePen(channelinspector::LabelColor);
	graph->valueAxis()->setAutoTicks(false);
	graph->valueAxis()->setAutoTickLabels(false);
	graph->valueAxis()->setSubTickCount(0);
	graph->valueAxis()->setLabelColor(channelinspector::LabelColor);
	graph->setPen(settings.value("display/plot-pens").toList().at(channel_).value<QPen>());

	/* Copy data from source graph */
	graph->setData(sourceGraph->data(), true);
	graph->rescaleValueAxis();
	plot->replot();

	/* Add the plot to the main window */
	layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(plot);
	setLayout(layout);
	setWindowTitle(QString("MeaView: Channel %1").arg(label));
	setAttribute(Qt::WA_DeleteOnClose);
	resize(channelinspector::WindowSize.first, channelinspector::WindowSize.second);
	saveFullPosition();

	if (settings.value("data/array").toString().startsWith("hidens")) {
		graph->valueAxis()->setLabel("uV");
	} else {
		graph->valueAxis()->setLabel("V");
	}

	/* Connect replot to the afterReplot signal of the source graph */
	QObject::connect(parentPlot, &QCustomPlot::afterReplot,
			this, &ChannelInspector::replot);
	QObject::connect(graph->valueAxis(), &QCPAxis::ticksRequest,
			this, [&] {
				/* Write 3 ticks at upper/lower range and center, but draw
				 * tick labels offset by that center (so center is 0)
				 */
				auto range = graph->valueAxis()->range();
				auto center = range.center();
				auto multiplier = settings.value("display/scale-multiplier").toDouble();
				ticks = { range.lower, center, range.upper };
				tickLabels = { 
						QString::number((range.lower - center) / multiplier, 'f', 3),
						QString::number(center / multiplier, 'f', 3), 
						QString::number((range.upper - center) / multiplier, 'f', 3)
					};
				graph->valueAxis()->setTickVector(ticks);
				graph->valueAxis()->setTickVectorLabels(tickLabels);
		});
}

ChannelInspector::~ChannelInspector()
{
	close();
}

void ChannelInspector::closeEvent(QCloseEvent* event) 
{
	emit aboutToClose(channel_);
	event->accept();
}

void ChannelInspector::replot() 
{
	graph->setData(sourceGraph->data(), true);
	graph->rescaleAxes();
	plot->replot();
}

int ChannelInspector::channel() 
{
	return channel_;
}

void ChannelInspector::updateSourceGraph(QCPGraph* source)
{
	sourceGraph = source;
}

void ChannelInspector::saveFullPosition()
{
	fullPos = geometry();
}

const QRect& ChannelInspector::fullPosition() const
{
	return fullPos;
}

}; // end channelinspector namespace
}; // end meaview namespace
