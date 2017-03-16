/*! \file subplot.cc
 *
 * Implemenation of Subplot class.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#include "subplot.h"

namespace meaview {
namespace subplot {

Subplot::Subplot(const int chan, const QString& label, 
		const int idx, const QPair<int, int>& pos,
		QCustomPlot* parent)
	: QObject(parent),
	m_channel(chan),
	m_label(label),
	m_index(idx),
	m_position(pos),
	m_ticks(3),
	m_tickLabels(3),
	m_mean(0.0)
{
	m_autoscale = (m_settings.value("data/array").toString().startsWith("hidens") ?
			false : plotwindow::McsAutoscaledChannels.contains(m_channel));

	m_pen = m_settings.value("display/plot-pens").toList().at(chan).value<QPen>();
	m_selectedPen = QPen{QColor::fromHsv(m_pen.color().hue(), 255, 255)};

	/* Create subplot axis and graph for the data */
	m_rect = new QCPAxisRect(parent); // parent will delete
	m_graph = parent->addGraph(m_rect->axis(QCPAxis::atBottom), 
			m_rect->axis(QCPAxis::atLeft));

	/* Format plot */
	graph()->keyAxis()->setTicks(false);
	graph()->keyAxis()->setTickLabels(false);
	graph()->keyAxis()->grid()->setVisible(false);
	graph()->keyAxis()->setRange(0, 
			(m_settings.value("sample-rate").toDouble() * 
			m_settings.value("display/refresh").toDouble()));
	graph()->keyAxis()->setLabel(m_label);
	graph()->keyAxis()->setLabelFont(subplot::LabelFont);
	graph()->keyAxis()->setLabelColor(subplot::LabelColor);
	graph()->keyAxis()->setLabelPadding(subplot::LabelPadding);
	graph()->keyAxis()->setBasePen(subplot::LabelColor);
	graph()->valueAxis()->setAutoTicks(false);
	graph()->valueAxis()->setAutoTickLabels(false);
	graph()->valueAxis()->setSubTickCount(0);
	graph()->valueAxis()->setTicks(false);
	graph()->valueAxis()->setTickLabels(false);
	graph()->valueAxis()->grid()->setVisible(false);
	graph()->valueAxis()->setBasePen(subplot::LabelColor);
	graph()->valueAxis()->setTickLabelColor(subplot::LabelColor);
	graph()->valueAxis()->setLabelFont(QFont{"Helvetica", 8, QFont::Light});
	auto scale = m_settings.value("display/scale").toDouble()
			* m_settings.value("display/scale-multiplier").toDouble();
	graph()->valueAxis()->setRange(-scale, scale);
}

Subplot::~Subplot()
{
}

void Subplot::addDataToBackBuffer(const QVector<DataFrame::DataType>& data, 
		QReadWriteLock* lock, const bool clicked)
{
	/* Transfer single data block to back buffer, update running mean */
	auto gain = m_settings.value("data/gain").toDouble();
	for (auto i = m_backBufferPosition; i < m_backBufferPosition + data.size(); i++) {
		auto point = gain * static_cast<double>(data.at(i - m_backBufferPosition));
		m_backBuffer.insert(i, QCPData(i, point));
		m_mean += point;
	}
	m_backBufferPosition += data.size();

	/* Full plot block available */
	if (m_backBufferPosition >= getPlotBlockSize()) {
		/* Remove any excess data */
		auto it = m_backBuffer.lowerBound(std::move(getPlotBlockSize()));
		while (it != m_backBuffer.end())
			it = m_backBuffer.erase(it);

		/* Swap buffers */
		lock->lockForRead();
		graph()->data()->swap(m_backBuffer);
		m_backBufferPosition = 0;
		formatPlot(clicked);
		lock->unlock();
		emit plotReady(index());
	}
}

void Subplot::formatPlot(bool clicked) 
{
	/* Set pen, brighter for selected plots. */
	if (clicked) {
		graph()->setPen(m_selectedPen);
	} else {
		graph()->setPen(m_pen);
	}

	if ( m_settings.value("display/autoscale").toBool() || m_autoscale ) {

		/* Auto scale this subplot's y-axis to fit the data. This is just
		 * done by rescaling the axis, and then drawing tick marks at
		 * the values corresponding to true voltage values.
		 */
		graph()->rescaleValueAxis();
		graph()->valueAxis()->setTicks(true);
		graph()->valueAxis()->setTickLabels(true);

		/* Write 3 ticks at upper/lower range and center, but draw
		 * tick labels offset by that center (so center is 0)
		 */
		auto range = graph()->valueAxis()->range();
		auto center = range.center();
		auto multiplier = m_settings.value("display/scale-multiplier").toDouble();
		m_ticks = { range.lower, center, range.upper };
		m_tickLabels = { 
				QString::number(((range.lower - center) / multiplier), 'f', 1),
				"0",
				QString::number(((range.upper - center) / multiplier), 'f', 1)
			};
		graph()->valueAxis()->setTickVector(m_ticks);
		graph()->valueAxis()->setTickVectorLabels(m_tickLabels);

	} else {

		/* Turn off ticks and set y-axis limits to the full scale. */
		m_mean /= getPlotBlockSize();
		graph()->valueAxis()->setTicks(false);
		graph()->valueAxis()->setTickLabels(false);
		auto scale = (m_settings.value("display/scale").toDouble()  *
				m_settings.value("display/scale-multiplier").toDouble());
		graph()->valueAxis()->setRange(m_mean - scale, m_mean + scale);
		m_mean = 0.0;

	}

	graph()->rescaleKeyAxis();
}

}; // end subplot namespace
}; // end meaview namespace

