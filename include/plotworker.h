/*! \file plotworker.h
 *
 * Header file describing worker class that transfers data and replots
 * in background threads.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#ifndef _MEAVIEW_PLOT_WORKER_H_
#define _MEAVIEW_PLOT_WORKER_H_

#include "settings.h"
#include "qcustomplot.h"
#include "subplot.h"

#include <QVector>
#include <QString>
#include <QSet>
#include <QReadWriteLock>
#include <QSettings>

namespace meaview {
namespace plotworker {

class PlotWorker : public QObject {
	Q_OBJECT

	public:
		/*! Construct a PlotWorker object */
		PlotWorker(QObject* parent = 0);
		PlotWorker(const PlotWorker& other) = delete;

		/*! Destroy a PlotWorker object */
		~PlotWorker();

		/*! Add responsibility for a single subplot to this worker */
		void addSubplot(subplot::Subplot* s);

	public slots:

		/*! Transfer data to this worker's subplot.
		 * \param subplot The subplot object to transfer data to.
		 * \param data The data to be transferred.
		 * \param lock The PlotWindow's read-write lock synchronizing access to
		 * the main plot object.
		 * \param clicked True if this subplot has been right-clicked, coloring
		 * the plot red.
		 */
		void transferDataToSubplot(subplot::Subplot* sp,
				QVector<double> data, QReadWriteLock* lock, const bool clicked);

		/*! Replot the data in this subplot.
		 * \param plot The parent QCustomPlot object owning all subplots.
		 * \param lock The read-write lock coordinating access to the QCustomPlot object.
		 * \param subplots All subplots in the main plot.
		 */
		void replot(QCustomPlot* plot, QReadWriteLock* lock, 
				QList<subplot::Subplot*> subplots);

		/*! Clear data the set of subplots, without deleting them */
		void clearSubplots();

	signals:

		/*! Emitted when this plot worker finishes redrawing the main QCustomPlot object */
		void plotUpdated(int nsamples);

	private:
		/*! Construct an X-axis of the given size.
		 * Because the amount of data plotted on any given refresh can change,
		 * with the changing refresh interval or the changing sample rate between
		 * the arrays, this data must be constructed on the fly.
		 */
		void constructXData(const int npoints);

		/*! The set of subplots managed by this worker */
		QSet<subplot::Subplot*> subplots;

		/*! True if subplots should be scaled to fit their data */
		bool autoscale;

		/*! X-axis data */
		QVector<double> xdata;

		/*! Global settings object */
		QSettings settings;

}; // end PlotWorker class

}; // end plotworker namespace
}; // end meaview namespace

#endif

