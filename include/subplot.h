/*! \file subplot.h
 *
 * Class for representing a single data subplot.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#ifndef _MEAVIEW_SUBPLOT_H_
#define _MEAVIEW_SUBPLOT_H_

#include "settings.h"
#include "qcustomplot.h"

#include "data-frame.h" // for DataFrame::DataType type alias

#include <QPair>
#include <QString>
#include <QSettings>
#include <QReadWriteLock>

namespace meaview {

namespace subplot {

/*! \class Subplot
 *
 * A subplot showing data from a single channel.
 *
 * The Subplot class manages the data shown from a single channel of
 * a recording. The class is intended to live in a background thread,
 * and mostly manages the transfer of new data from the BLDS to the
 * QCustomPlot graph actually showing the data. Because the class lives
 * in background threads, signals and slots are provided to send it
 * new data and to receive notifications of when the transfer has 
 * completed.
 *
 * Although the Subplot creates and maintains a reference to the 
 * graph, axes, and other QCustomPlot-related objects, it does *not*
 * actually own or delete them. They are created as children of the
 * main PlotWindow's QCustomPlot object. An instance of the Subplot
 * class should always be deleted through the `requestDelete` slot,
 * and any of the above objects (graphs etc) should not be deleted
 * until the `deleted()` signal has been received. This ensures that
 * the Subplot has released any references to these objects, and that
 * no races occur.
 */
class Subplot : public QObject {
	Q_OBJECT

	public:
		/*! Construct a Subplot.
		 * \param channel The channel number this plot will represent.
		 * \param label The label drawn on this subplot
		 * \param subplotIndex The linear index of the subplot where data is plotted.
		 * \param position The x- and y-position of the subplot in the grid.
		 * \param plot The parent QCustomPlot object
		 */
		Subplot(int channel, const QString& label,
				int subplotIndex, const QPair<int, int>& position,
				QCustomPlot* plot);

		/*! Destroy a Subplot.
		 *
		 * NOTE: Although the Subplot creates and maintains references to
		 * various plot-related objects (graphs, ticks, etc), these *must not*
		 * be deleted in this destructor. Those objects are owned by the
		 * parent QCustomPlot, and will be deleted by either that destructor
		 * or the various methods in the PlotWindow class.
		 */
		~Subplot();

		/*! Return the data channel number this subplot represents */
		inline int channel() const { return m_channel; }

		/*! Return the subplot index for this Subplot */
		inline int index() const { return m_index; }

		/*! Return this Subplot's axis label */
		inline const QString& label() const { return m_label; }

		/*! Return the x- and y-position of this subplot in the current plot grid */
		inline const QPair<int, int>& position() const { return m_position; }

		/*! Set the x- and y-position of this subplot in the current plot grid.
		 * This function does _not_ move the subplot to this position. That is
		 * performed by the parent PlotWindow object.
		 */
		inline void setPosition(const QPair<int, int>& p) { m_position = p; }

		/*! Return the graph object responsible for managing and plotting 
		 * the actual channel data.
		 */
		inline QCPGraph* graph() const { return m_graph; } 

		/*! Return the axis rectange in which this Subplot draws 
		 * its data
		 */
		inline QCPAxisRect* rect() const { return m_rect; }

		/*! Format this subplot for plotting, e.g. rescale axes and set pens.  */
		void formatPlot(bool clicked);

		/*! Compare two subplots for equality.
		 * Subplots are considered equal if they live at the same linear index
		 * in the main plot grid.
		 */
		inline bool operator==(const Subplot& other) const
		{
			return index() == other.index();
		}

		/*! Compare two subplots for inequality.
		 * Subplots are considered equal if they live at the same linear index
		 * in the main plot grid.
		 */
		inline bool operator!=(const Subplot& other) const
		{
			return (!(*this == other));
		}

	signals:

		/*! Emitted when this subplot is ready to be replotted, with this
		 * subplot's index as the argument.
		 *
		 * This occurs when the back buffer has been filled with enough
		 * data for a full plot block, but before the front and back 
		 * buffers have been swapped.
		 */
		void plotReady(int idx, int npoints);

		/*! Emitted just before the subplot is deleted.
		 *
		 * This is used to communicate with the main PlotWindow object,
		 * which waits for all subplots to be deleted before clearing
		 * the plot grid and graphs.
		 */
		void deleted(int idx);

	public slots:

		/*! Add new data to the subplot.
		 *
		 * \param sp The intended subplot for this array of data.
		 * \param data The actual data.
		 * \param lock Read-write lock used to synchronize access with the main
		 * 	PlotWindow object for redrawing the plots.
		 * \param clicked True if this plot was clicked, and false otherwise.
		 *
		 * This method adds data to the subplot's back buffer, and if enough
		 * data has been accumulated to warrant a replot, this formats the plot
		 * (e.g, scaling axes) and notifies the main PlotWindow that this subplot
		 * is ready to be replotted.
		 */
		void handleNewData(Subplot* sp, QVector<DataFrame::DataType>* data,
				QReadWriteLock* lock, bool clicked);

		/*! Request that the subplot be deleted. */
		void requestDelete();

		/*! Called when the refresh rate of the plot is changed,
		 * indicating that the number of samples before refreshing
		 * has changed.
		 */
		void updatePlotBlockSize();

	private:

		/* Actual channel number for this subplot. */
		int m_channel;

		/* True if this subplot should autoscale its y-axis to fit its data. */
		bool m_autoscale;

		/* String labeling this channel. Often but not always the same
		 * as m_channel.
		 */
		QString m_label;

		/* Index into the grid of subplots for this channel. */
		int m_index;

		/* Row, column position in the grid of subplots. */
		QPair<int, int> m_position;

		/* Graph containing the raw data for this subplot. */
		QCPGraph* m_graph;

		/* Axis rectangle for the subplot. */
		QCPAxisRect* m_rect;

		/* Back buffer, into which data is written in a background
		 * thread. This allows data to be transferred to the subplot while
		 * the main plot is still updating.
		 */
		QCPDataMap m_backBuffer;

		/* Current position in the back buffer. */
		int m_backBufferPosition = 0;

		/* Global settings. */
		QSettings m_settings;

		/* Tick positions for the y-axis. */
		QVector<double> m_ticks;

		/* Labels for the y-axis tick marks. */
		QVector<QString> m_tickLabels;

		/* Pen used to draw data. */
		QPen m_pen;

		/* Pen used to draw data when this plot has been clicked/selected. */
		QPen m_selectedPen;

		/* Return the number of samples in a plot block. */
		int m_plotBlockSize;
};

}; // end subplot namespace
}; // end meaview namespace

#endif

