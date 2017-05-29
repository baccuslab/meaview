/*! \file subplot.cc
 *
 * Implemenation of Subplot class.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#include "subplot.h"

namespace meaview {
namespace subplot {

Subplot::Subplot(int chan, const QString& label, 
		int idx, const QPair<int, int>& pos,
		QCustomPlot* parent)
	: QObject(nullptr),
	m_channel(chan),
	m_label(label),
	m_index(idx),
	m_position(pos),
	m_ticks(3),
	m_tickLabels(3)
{
	m_autoscale = (m_settings.value("data/array").toString().startsWith("hidens") ?
			false : plotwindow::McsAutoscaledChannels.contains(m_channel));

	m_pen = m_settings.value("display/plot-pens").toList().at(chan).value<QPen>();
	m_selectedPen = QPen{QColor::fromHsv(m_pen.color().hue(), 255, 255)};

	updatePlotBlockSize();

	/* Create subplot axis and graph for the data */
	m_rect = new QCPAxisRect(parent); // parent will delete
	m_graph = parent->addGraph(m_rect->axis(QCPAxis::atBottom), 
			m_rect->axis(QCPAxis::atLeft)); // parent will delete

	/* Format plot */
	auto keyAxis = m_graph->keyAxis();
	keyAxis->setTicks(false);
	keyAxis->setTickLabels(false);
	keyAxis->grid()->setVisible(false);
	keyAxis->setRange(0, 
			(m_settings.value("data/sample-rate").toDouble() * 
			m_settings.value("display/refresh").toDouble()));
	keyAxis->setLabel(m_label);
	keyAxis->setLabelFont(subplot::LabelFont);
	keyAxis->setLabelColor(subplot::LabelColor);
	keyAxis->setLabelPadding(subplot::LabelPadding);
	keyAxis->setBasePen(subplot::LabelColor);

	auto valueAxis = m_graph->valueAxis();
	valueAxis->setAutoTicks(false);
	valueAxis->setAutoTickLabels(false);
	valueAxis->setSubTickCount(0);
	valueAxis->setTicks(false);
	valueAxis->setTickLabels(false);
	valueAxis->grid()->setVisible(false);
	valueAxis->setBasePen(subplot::LabelColor);
	valueAxis->setTickLabelColor(subplot::LabelColor);
	valueAxis->setLabelFont(QFont{"Helvetica", 8, QFont::Light});
	auto scale = m_settings.value("display/scale").toDouble()
			* m_settings.value("display/scale-multiplier").toDouble();
	valueAxis->setRange(-scale, scale);
}

Subplot::~Subplot()
{
}

void Subplot::requestDelete()
{
	deleteLater();
	emit deleted(m_index);
}

void Subplot::updatePlotBlockSize()
{
	m_plotBlockSize = static_cast<int>(
			m_settings.value("display/refresh").toDouble() *
			m_settings.value("data/sample-rate").toDouble());
}

void Subplot::handleNewData(Subplot* sp, QVector<DataFrame::DataType>* data, 
		QReadWriteLock* lock, const bool clicked)
{
	if (sp != this)
		return;

	/* Transfer single data block to back buffer. */
	auto gain = m_settings.value("data/gain").toDouble();
	for (auto i = m_backBufferPosition; i < m_backBufferPosition + data->size(); i++) {
		auto point = gain * static_cast<double>(
				data->at(i - m_backBufferPosition));
		m_backBuffer.insert(i, QCPData(i, point));
	}
	m_backBufferPosition += data->size();

	/* Full plot block available */
	if (m_backBufferPosition >= m_plotBlockSize) {

		/* Remove any excess data */
		auto it = m_backBuffer.lowerBound(m_plotBlockSize);
		while (it != m_backBuffer.end())
			it = m_backBuffer.erase(it);

		/* Swap front and back buffers and reformat the plot. */
		lock->lockForRead();
		m_graph->data()->swap(m_backBuffer);
		m_backBufferPosition = 0;
		formatPlot(clicked);
		lock->unlock();
		emit plotReady(m_index, m_plotBlockSize);
	}
	delete data;
}

void Subplot::formatPlot(bool clicked) 
{
	/* Set pen, brighter for selected plots. */
	if (clicked) {
		m_graph->setPen(m_selectedPen);
	} else {
		m_graph->setPen(m_pen);
	}

	if ( m_settings.value("display/autoscale").toBool() || m_autoscale ) {

		/* Auto scale this subplot's y-axis to fit the data. This is just
		 * done by rescaling the axis, and then drawing tick marks at
		 * the values corresponding to true voltage values.
		 */
		m_graph->rescaleValueAxis();
		m_graph->valueAxis()->setTicks(true);
		m_graph->valueAxis()->setTickLabels(true);

		/* Write 3 ticks at upper/lower range and center, but draw
		 * tick labels offset by that center (so center is 0)
		 */
		auto range = m_graph->valueAxis()->range();
		auto center = range.center();
		auto multiplier = m_settings.value("display/scale-multiplier").toDouble();
		m_ticks = { range.lower, center, range.upper };
		m_tickLabels = { 
				QString::number(((range.lower - center) / multiplier), 'f', 1),
				"0",
				QString::number(((range.upper - center) / multiplier), 'f', 1)
			};
		m_graph->valueAxis()->setTickVector(m_ticks);
		m_graph->valueAxis()->setTickVectorLabels(m_tickLabels);

	} else {

		/* Compute mean. */
		auto mean = 0.0;
		auto data = m_graph->data();
		for (auto i = 0; i < data->size(); i++) {
			mean += data->value(i).value;
		}
		mean /= data->size();

		/* Turn off ticks and set y-axis limits to the full scale. */
		m_graph->valueAxis()->setTicks(false);
		m_graph->valueAxis()->setTickLabels(false);
		auto scale = (m_settings.value("display/scale").toDouble()  *
				m_settings.value("display/scale-multiplier").toDouble());
		m_graph->valueAxis()->setRange(mean - scale, mean + scale);

	}

	m_graph->rescaleKeyAxis();
}

}; // end subplot namespace
}; // end meaview namespace

