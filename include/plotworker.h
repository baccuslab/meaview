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

#include "data-frame.h" // for DataFrame::DataType type alias

#include <QVector>
#include <QString>
#include <QSet>
#include <QReadWriteLock>
#include <QSettings>

namespace meaview {
namespace plotworker {

/*! \class PlotWorker
 *
 * The PlotWorker class is a small class which manages a unique list
 * of subplots in the current plot window, and which transfers data
 * to those subplots in separate threads.
 *
 * This works by giving the PlotWorker class access directly to the
 * Subplot class's back buffer object, to which the worker transfers
 * data. Once this back buffer is full enough for a replot, the worker
 * locks the main QCustomPlot's read-write lock, swaps the front and
 * back buffers (which is very fast), and then emits a signal indicating
 * that the Subplot has been updated and is ready for replot.
 *
 * The main PlotWindow object is notified of these ready signals, and
 * once all of the Subplot's have been updated, replots the whole 
 * main plot surface.
 *
 * \note This seemingly-complicated design leads to the question of
 * why not just put the Subplot's themselves in background threads,
 * and have them transfer data. The main reason is that it is best
 * to keep all GUI-related objects in the main thread in Qt. That
 * solution may work, and may be implemented in the future, but it
 * was deemed necessary to keep the plot widgets entirely in the 
 * main thread, and just give these PlotWorker objects access to
 * the Subplot.
 *
 * In other words, this class is really here so that the method
 * Subplot::addDataToBackBuffer() can be run in a separate thread.
 */
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
				QVector<DataFrame::DataType> data, 
				QReadWriteLock* lock, const bool clicked);

		/*! Clear data the set of subplots, without deleting them */
		void clearSubplots();

	private:
		/*! Construct an X-axis of the given size.
		 *
		 * \param npoints The number of points to create in the new x-axis.
		 *
		 * Because the amount of data plotted on any given refresh can change,
		 * with the changing refresh interval or the changing sample rate between
		 * the arrays, this data must be constructed on the fly.
		 */
		void constructXData(int npoints);

		/*! The set of subplots managed by this worker.
		 *
		 * As noted in the class documentation, this is the set of subplots
		 * that this worker transfers data to in its separate thread.
		 */
		QSet<subplot::Subplot*> m_subplots;

		/*! True if subplots should be scaled to fit their data */
		bool m_autoscale;

		/*! X-axis data */
		QVector<double> m_xdata;

		/*! Global settings object */
		QSettings m_settings;

}; // end PlotWorker class

}; // end plotworker namespace
}; // end meaview namespace

#endif

