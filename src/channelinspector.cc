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
	m_ticks(3),
	m_tickLabels(3)
{
	m_channel = chan;
	m_sourceGraph = source;

	/* Create plot axis, graph, etc. */
	m_plot = new QCustomPlot(this);
	m_plot->setBackground(channelinspector::BackgroundColor);
	m_graph = m_plot->addGraph();
	m_graph->keyAxis()->setTicks(false);
	m_graph->keyAxis()->setTickLabels(false);
	m_graph->keyAxis()->grid()->setVisible(false);
	m_graph->keyAxis()->setRange(0, m_settings.value("data/sample-rate").toDouble() *
			m_settings.value("display/refresh").toDouble());
	m_graph->keyAxis()->setBasePen(channelinspector::LabelColor);
	m_graph->valueAxis()->setTickLabelColor(channelinspector::LabelColor);
	m_graph->valueAxis()->grid()->setVisible(false);
	m_graph->valueAxis()->setBasePen(channelinspector::LabelColor);
	m_graph->valueAxis()->setAutoTicks(false);
	m_graph->valueAxis()->setAutoTickLabels(false);
	m_graph->valueAxis()->setSubTickCount(0);
	m_graph->valueAxis()->setLabelColor(channelinspector::LabelColor);
	m_graph->setPen(m_settings.value("display/plot-pens").toList().at(m_channel).value<QPen>());

	/* Copy data from source graph */
	m_graph->setData(m_sourceGraph->data(), true);
	m_graph->rescaleValueAxis();
	m_plot->replot();

	/* Add the plot to the main window */
	m_layout = new QGridLayout(this);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->addWidget(m_plot);
	setLayout(m_layout);
	setWindowTitle(QString("Meaview inspector: Channel %1").arg(label));
	setAttribute(Qt::WA_DeleteOnClose);
	resize(channelinspector::WindowSize.first, channelinspector::WindowSize.second);
	saveFullPosition();

	if (m_settings.value("data/array").toString().startsWith("hidens")) {
		m_graph->valueAxis()->setLabel("uV");
	} else {
		m_graph->valueAxis()->setLabel("V");
	}

	/* Connect replot to the afterReplot signal of the source graph */
	QObject::connect(parentPlot, &QCustomPlot::afterReplot,
			this, &ChannelInspector::replot);
	QObject::connect(m_graph->valueAxis(), &QCPAxis::ticksRequest,
			this, [&] {
				/* Write 3 ticks at upper/lower range and center, but draw
				 * tick labels offset by that center (so center is 0)
				 */
				auto range = m_graph->valueAxis()->range();
				auto center = range.center();
				auto multiplier = m_settings.value("display/scale-multiplier").toDouble();
				m_ticks = { range.lower, center, range.upper };
				m_tickLabels = { 
						QString::number((range.lower - center) / multiplier, 'f', 3),
						QString::number(center / multiplier, 'f', 3), 
						QString::number((range.upper - center) / multiplier, 'f', 3)
					};
				m_graph->valueAxis()->setTickVector(m_ticks);
				m_graph->valueAxis()->setTickVectorLabels(m_tickLabels);
		});
}

ChannelInspector::~ChannelInspector()
{
	close();
}

void ChannelInspector::closeEvent(QCloseEvent* event) 
{
	emit aboutToClose(m_channel);
	event->accept();
}

void ChannelInspector::replot() 
{
	m_graph->setData(m_sourceGraph->data(), true);
	m_graph->rescaleAxes();
	m_plot->replot();
}

int ChannelInspector::channel() 
{
	return m_channel;
}

void ChannelInspector::saveFullPosition()
{
	m_fullPos = geometry();
}

const QRect& ChannelInspector::fullPosition() const
{
	return m_fullPos;
}

}; // end channelinspector namespace
}; // end meaview namespace

