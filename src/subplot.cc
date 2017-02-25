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
	channel_(chan),
	label_(label),
	index_(idx),
	position_(pos),
	ticks(3),
	tickLabels(3),
	mean(0.0)
{
	isAutoscaleChannel = (settings.value("data/array").toString().startsWith("hidens") ?
			false : plotwindow::McsAutoscaledChannels.contains(channel_));

	pen = settings.value("display/plot-pens").toList().at(chan).value<QPen>();
	selectedPen = QPen{QColor::fromHsv(pen.color().hue(), 255, 255)};

	/* Create subplot axis and graph for the data */
	rect_ = new QCPAxisRect(parent); // parent will delete
	graph_ = parent->addGraph(rect_->axis(QCPAxis::atBottom), 
			rect_->axis(QCPAxis::atLeft));

	/* Format plot */
	graph()->keyAxis()->setTicks(false);
	graph()->keyAxis()->setTickLabels(false);
	graph()->keyAxis()->grid()->setVisible(false);
	graph()->keyAxis()->setRange(0, 
			(settings.value("sample-rate").toDouble() * 
			settings.value("display/refresh").toDouble() / 1000));
	graph()->keyAxis()->setLabel(label_);
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
	auto scale = settings.value("display/scale").toDouble()
			* settings.value("display/scale-multiplier").toDouble();
	graph()->valueAxis()->setRange(-scale, scale);
}

Subplot::~Subplot()
{
}

void Subplot::addDataToBackBuffer(const QVector<double>& data, QReadWriteLock* lock, 
		const bool clicked)
{
	/* Transfer single data block to back buffer, update running mean */
	for (auto i = backBufferPosition; i < backBufferPosition + data.size(); i++) {
		backBuffer.insert(i, QCPData(i, data.at(i - backBufferPosition)));
		mean += data.at(i - backBufferPosition);
	}
	backBufferPosition += data.size();

	/* Full plot block available */
	if (backBufferPosition >= getPlotBlockSize()) {
		/* Remove any excess data */
		auto it = backBuffer.lowerBound(std::move(getPlotBlockSize()));
		while (it != backBuffer.end())
			it = backBuffer.erase(it);

		/* Swap buffers */
		lock->lockForRead();
		graph()->data()->swap(backBuffer);
		backBufferPosition = 0;
		formatPlot(clicked);
		lock->unlock();
		emit plotReady(index());
	}
}

void Subplot::formatPlot(bool clicked) {
	if (clicked) {
		graph()->setPen(selectedPen);
	} else {
		graph()->setPen(pen);
	}
	if ( settings.value("display/autoscale").toBool() || isAutoscaleChannel ) {
		graph()->rescaleValueAxis();
		graph()->valueAxis()->setTicks(true);
		graph()->valueAxis()->setTickLabels(true);

		/* Write 3 ticks at upper/lower range and center, but draw
		 * tick labels offset by that center (so center is 0)
		 */
		auto range = graph()->valueAxis()->range();
		auto center = range.center();
		auto multiplier = settings.value("display/scale-multiplier").toDouble();
		ticks = { range.lower, center, range.upper };
		tickLabels = { 
				QString::number(static_cast<int>((range.lower - center) / multiplier)),
				"0",
				QString::number(static_cast<int>((range.upper - center) / multiplier))
			};
		graph()->valueAxis()->setTickVector(ticks);
		graph()->valueAxis()->setTickVectorLabels(tickLabels);
	} else {
		mean /= getPlotBlockSize();
		graph()->valueAxis()->setTicks(false);
		graph()->valueAxis()->setTickLabels(false);
		auto scale = (settings.value("display/scale").toDouble()  *
				settings.value("display/scale-multiplier").toDouble());
		graph()->valueAxis()->setRange(mean - scale, mean + scale);
		mean = 0.0;
	}

	graph()->rescaleKeyAxis();
}

}; // end subplot namespace
}; // end meaview namespace

