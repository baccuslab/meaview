/*! \file plotwindow.h
 *
 * Header file defining window for displaying plots of array data.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#ifndef _MEAVIEW_PLOT_WINDOW_H_
#define _MEAVIEW_PLOT_WINDOW_H_

#include "settings.h"
#include "qcustomplot.h"
#include "plotworker.h"
#include "channelinspector.h"
#include "subplot.h"

#include "data-frame.h"

#include <QGridLayout>
#include <QPoint>
#include <QList>
#include <QThread>
#include <QReadWriteLock>
#include <QSet>
#include <QStringList>
#include <QPair>

namespace meaview {
namespace plotwindow {

class PlotWindow : public QWidget {
	Q_OBJECT

	public:
		PlotWindow(QWidget* parent = 0);
		~PlotWindow();

		/*! Set up the plot window to accept data from an array, with a
		 * given number of channels. Initially, the window will contain
		 * a simple square grid that is large enough to hold the number
		 * of channels.
		 *
		 * \param array The type of array from which data is recorded.
		 * \param nchannels The number of expected channels in the data.
		 */
		void setupWindow(const QString& array, const int nchannels);

		/*! Transfer data contained in an Armadillo matrix into
		 * the corresponding channel subplots
		 */
		void transferDataToSubplots(const DataFrame::Samples& samples);
		void transferDataToSubplots(const DataFrame& samples);

		/*! Return the currently-used channel view */
		const plotwindow::ChannelView& currentView() const;

	signals:
		/*! Send data to a plot worker, in a separate thread, for plotting.
		 * \param subplot The subplot in which the data will be plotted.
		 * \param data The actual data being plotted.
		 * \param lock The read-write lock synchronizing access to the main plot
		 * \param clicked true if the user right-clicked this plot, coloring it red.
		 */
		void sendDataToPlotWorker(subplot::Subplot* subplot, 
				QVector<double> data, QReadWriteLock* lock, bool clicked);

		/*! Emitted when data in all subplots has been updated, so that the main window
		 * can refresh itself.
		 * \param p The main plot object containing all subplots. This is what actually
		 * does the plot refresh.
		 * \param lock The read-write lock coordinating access to the QCustomPlot object
		 * \param subplots The list of all subplots.
		 */
		void allSubplotsUpdated(QCustomPlot* p, QReadWriteLock* lock, 
				QList<subplot::Subplot*> subplots);

		/*! Emitted when the number of open inspectors changes.
		 * \param num The number of currently open inspectors.
		 */
		void numInspectorsChanged(int n);

		/*! Emitted when the window wants to hide, which is an overridden behavior
		 * for a non-sponaneous close event (e.g., user clicks the close button,
		 * rather than doing Ctrl-Q)
		 */
		void hideRequest();

		/*! Emitted when the plot itself is refreshed, with the number of samples
		 * contained in the plots
		 */
		void plotRefreshed(int nsamples);

		/*! Emitted to notify plot worker threads that they should completely clear
		 * and delete their subplots
		 */
		void clearSubplots();

	public slots:
		/*! Toggles whether the plot window is visible */
		void toggleVisible();
		
		/*! Toggles whether the window is in its minimal state */
		void minify(bool min);

		/*! Clears all data from all subplots */
		void clearAllData();

		/*! Completely clears the plot window, including all axes, graphs, etc */
		void clear();

		/*! Increments the number of subplots that have finished transferring data.
		 * This is used to notify the main plot window when it can redraw itself.
		 */
		void incrementNumPlotsUpdated(const int idx);

		/*! Create a new window dedicated to a single plot, for detailed inspection
		 * of its data. This is useful for an intracellular recording, for example.
		 */
		void createChannelInspector(QMouseEvent* event);

		/*! Handle a click on a single channel. Clicks can be used to color plots
		 * red (to keep track of them), or to open an inspector window for a detailed
		 * view of the channel's data.
		 */
		void handleChannelClick(QMouseEvent* event);

		/*! Update the mapping between channel indices and subplots, giving the data 
		 * the arrangement of the actual electrodes from which it originates.
		 */
		void updateChannelView();

		/*! Block or unblock resizing of the main plot window.
		 * This is used to prevent segfaults that I'm as yet unable to diagnose, 
		 * which come from a graphics backing store being deleted as data is transferred
		 * or a plot is refreshed.
		 */
		void blockResize(const bool block);

		/*! Toggle whether all channel inspector windows are visible */
		void toggleInspectorsVisible();

		/*! Handle cleanup after a plot is completely updated */
		void handlePlotUpdated(const int numNewPoints);

	private:

		/*! Construct a pool of plotting threads */
		void initThreadPool();

		/*! Initialize the main QCustomPlot object */
		void initPlot();

		/*! Assign subplots to plot workers.
		 * As new data is loaded or streamed, the number of subplots may change.
		 * This function assigns each subplot to PlotWorkers, in as equitable a
		 * manner as possible
		 */
		void assignSubplotsToWorkers();

		/*! Return the subplot containing the given position */
		subplot::Subplot* findSubplotContainingPoint(const QPoint& point);

		/*! Destroy a channel inspector window */
		void removeChannelInspector(const int channel);

		/*! Compute the size of a plot grid layout needed for the current
		 * view on the current array.
		 */
		void computePlotGridSize();

		/*! Create a plot layout of the appropriate size, first removing any
		 * existing elements from the layout.
		 */
		void createPlotGrid();

		/*! Construct or load a view for the current array. A view is a
		 * mapping from data channel number (and thus subplot index) to
		 * the (row, col) of the current subplot grid layout.
		 */
		void createChannelView();

		/*! Assign subplots to their position in the current plot grid,
		 * given the current view.
		 */
		void assignSubplotsToGrid();

		/*! Update the subplot source graph for any open channel inspectors */
		void updateInspectorSources();

		/*! Move subplots to a cell in the current subplot grid defined by
		 * the current channel view
		 */
		void moveSubplots();

		/*! Override close event to just hide the window */
		void closeEvent(QCloseEvent* ev);

		/*! Stack inspectors when minifying */
		void stackInspectors();

		/*! Return inspectors to original position */
		void unstackInspectors();

		/*! Compute colors equally spaced around color circle for this 
		 * number of channels.
		 */
		void computePlotColors();

		/*! Number of plot transfer threads */
		const int nthreads = QThread::idealThreadCount();

		/*! The window's size prior to minifying. This is used to 
		 * reset the window position when un-minifying
		 */
		QRect fullPosition;

		/*! Size of subplot grid, {rows, columns} */
		QPair<int, int> gridSize;

		/*! Number of total subplots */
		int nsubplots;

		/*! Bit array representing the plots whose front and back buffers
		 * have been swapped, and so are ready for a replot.
		 */
		QBitArray subplotsUpdated;

		/*! Labels for each channel */
		QStringList channelLabels;

		/*! The mapping between channel index and subplot position */
		plotwindow::ChannelView view;

		/*! Set containing plots that have been clicked */
		QSet<subplot::Subplot*> clickedPlots;

		/*! List of active channel inspector windows */
		QList<channelinspector::ChannelInspector*> inspectors;
		
		/*! Global settings */
		QSettings settings;

		/*! Main layout for the subplot grid */
		QGridLayout* layout;

		/*! Main plot object, containing all subplots */
		QCustomPlot* plot;

		/*! List of all subplots */
		QList<subplot::Subplot*> subplots;

		/*! List of all plot transfer threads */
		QList<QThread*> threads;

		/*! List of all plot worker objects, which transfer data to the actual subplots */
		QList<plotworker::PlotWorker*> workers;

		/*! The plot worker whose sole job is to redraw the main QCustomPlot object. */
		plotworker::PlotWorker* replotWorker;

		/*! The thread in which teh replotWorker lives */
		QThread* replotThread;

		/*! Read-write lock for the main plot.\
		 * This is the only synchronization primitive used to coordinate the plotting threads.
		 * The threads transferring data to back buffers may proceed at any time, and only
		 * their swaps of the subplots' front and back buffers need to be synchronized with
		 * the main plot's redraw.
		 */
		QReadWriteLock lock;

		/*! True if resizes are blocked on this window */
		bool resizeBlocked = false;

}; // end PlotWindow class

}; // end plotwindow namespace
}; // end meaview namespace

#endif

