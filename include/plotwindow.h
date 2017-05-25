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

/*! \class PlotWindow
 * Main widget containing grid of data plots.
 *
 * The PlotWindow class is a widget used to contain, display, and manage
 * a grid of subplots showing data from each channel. It manages positioning
 * each subplot appropriately in the grid, moving them around if necessary.
 * The PlotWindow also sends data from the appropriate channel to the 
 * correct subplot displaying that data. The PlotWindow provides
 * functionality for creating a "channel inspector", a blown-up view of
 * a single channel of data.
 */
class PlotWindow : public QWidget {
	Q_OBJECT

	public:
		/*! Construct a PlotWindow. */
		PlotWindow(QWidget* parent = 0);

		/*! Destroy the plot window. */
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

		/*! Return the currently-used channel view */
		const plotwindow::ChannelView& currentView() const;

	signals:

		/*! Send data to a subplot for transfer.
		 *
		 * \param sp The subplot in which the data will be plotted.
		 * \param data Vector of data to be transferred.
		 * \param lock The read-write lock synchronizing access to the main plot surface.
		 * \param clicked True if the user has right-clicked this plot.
		 *
		 * NOTE: The data is passed as a pointer, and the subplot which actually
		 * contains the data *must* delete it.
		 */
		void sendDataToSubplot(subplot::Subplot* sp, 
				QVector<DataFrame::DataType>* data, QReadWriteLock* lock,
				bool clicked);

		/*! Emitted when the number of open inspectors changes.
		 * \param num The number of currently open inspectors.
		 */
		void numInspectorsChanged(int num);

		/*! Emitted when the plot has been redrawn with a new chunk of data.
		 *
		 * \param nsamples The number of samples plotted in each subplot.
		 */
		void plotRefreshed(int nsamples);

		/*! Emitted when all subplots have been cleared and deleted, and the
		 * grid layout is deleted. This indicates that the grid of plots is
		 * ready to be re-created.
		 */
		void cleared();

		/*! Emitted to notify all subplots that they should be deleted. */
		void deleteSubplots();

	public slots:

		/*! Minify the plot window and any open channel inspectors. */
		void minify(bool min);

		/*! Completely clears the plot window, including all axes, graphs, etc */
		void clear();

		/*! Update the mapping between channel indices and subplots, giving the data 
		 * the arrangement of the actual electrodes from which it originates.
		 */
		void updateChannelView();

		/*! Toggle whether all channel inspector windows are visible */
		void toggleInspectorsVisible();

	private slots:

		/*! Handle the subplot with the following index being deleted. */
		void handleSubplotDeleted(int index);

		/*! Handle a click on a single channel. Clicks can be used to color plots
		 * red (to keep track of them), or to open an inspector window for a detailed
		 * view of the channel's data.
		 */
		void handleChannelClick(QMouseEvent* event);

		/*! Create a new window dedicated to a single plot, for detailed inspection
		 * of its data. This is useful for an intracellular recording, for example.
		 */
		void createChannelInspector(QMouseEvent* event);

		/*! Increments the number of subplots that have finished transferring data.
		 * This is used to notify the main plot window when it can redraw itself.
		 */
		void incrementNumPlotsUpdated(const int idx);

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

		/*! Move subplots to a cell in the current subplot grid defined by
		 * the current channel view
		 */
		void moveSubplots();

		/*! Stack inspectors when minifying */
		void stackInspectors();

		/*! Return inspectors to original position */
		void unstackInspectors();

		/*! Compute colors equally spaced around color circle for this 
		 * number of channels.
		 */
		void computePlotColors(const QMap<int, bool>& valid);

		void replot();

		void handleAllSubplotsDeleted();

		/*! Compute which channels carry valid data. */
		QMap<int, bool> computeValidDataChannels();

		/*! Number of plot transfer threads */
		const int nthreads = QThread::idealThreadCount();

		/*! Size of subplot grid, {rows, columns} */
		QPair<int, int> gridSize;

		/*! Number of total subplots */
		int nsubplots;

		/*! Bit array representing the subplots whose front and back buffers
		 * have been swapped, and so are ready for a replot.
		 */
		QBitArray subplotsUpdated;

		/*! Bit array representing the subplots which have
		 * been deleted. This is used to clear the plot window after
		 * all have been deleted.
		 */
		QBitArray subplotsDeleted;

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
		QList<QThread*> transferThreads;

		/*! Read-write lock for the main plot.
		 * This is the only synchronization primitive used to coordinate 
		 * the transfer threads. The threads transferring data to back 
		 * buffers may proceed at any time, and only their swaps of the 
		 * subplots' front and back buffers need to be synchronized with
		 * the main plot's redraw.
		 */
		QReadWriteLock lock;

}; // end PlotWindow class

}; // end plotwindow namespace
}; // end meaview namespace

#endif

