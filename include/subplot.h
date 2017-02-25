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

#include <QPair>
#include <QString>
#include <QSettings>
#include <QReadWriteLock>

namespace meaview {

namespace subplot {

/*! A Subplot class represents a single data subplot, including
 * the data channel from which data is drawn. The Subplot also
 * contains the current (row, col) in the parent QCustomPlot's 
 * grid layout, and a label for the subplot.
 *
 * Note that a Subplot always represents a single data channel,
 * which is constant over the lifetime of the object. However, 
 * the _position_ of the subplot in its parent grid layout may
 * change, if the selected channel view changes.
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
		Subplot(const int channel, const QString& label,
				const int subplotIndex, const QPair<int, int>& position,
				QCustomPlot* plot);

		/*! Destroy a subplot, deleting all children and data */
		~Subplot();

		/*! Return the data channel number this subplot represents */
		inline int channel() const { return channel_; }

		/*! Return the subplot index for this Subplot */
		inline int index() const { return index_; }

		/*! Return this Subplot's axis label */
		inline const QString& label() const { return label_; }

		/*! Return the x- and y-position of this subplot in the current plot grid */
		inline const QPair<int, int>& position() const { return position_; }

		/*! Set the x- and y-position of this subplot in the current plot grid.
		 * This function does _not_ move the subplot to this position. That is
		 * performed by the parent PlotWindow object.
		 */
		inline void setPosition(const QPair<int, int>& p) { position_ = p; }

		/*! Return the graph object responsible for managing and plotting 
		 * the actual channel data.
		 */
		inline QCPGraph* graph() const { return graph_; } 

		/*! Return the axis rectange in which this Subplot draws 
		 * its data
		 */
		inline QCPAxisRect* rect() const { return rect_; }

		/*! Add data to the subplot's back buffer */
		void addDataToBackBuffer(const QVector<double>& data, QReadWriteLock* lock,
				const bool clicked);

		/*! Format this subplot for plotting, e.g. rescale axes and set pens.  */
		void formatPlot(bool clicked);

		/*! Clear all data in the front and back buffers */
		inline void clearData()
		{
			graph()->clearData();
			backBuffer.clear();
			backBufferPosition = 0;
		}

		/*! Return the size of the back buffer, used to determine 
		 * when the subplot has enough data to plot a full block.
		 */
		inline int backBufferSize() const { return backBufferPosition; }

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
		 * This occurs when the back buffer has been filled with enough
		 * data for a full plot block, but before the front and back 
		 * buffers have been swapped.
		 */
		void plotReady(int idx);

	private:
		int channel_;
		bool isAutoscaleChannel;
		QString label_;
		int index_;
		QPair<int, int> position_;
		QCPGraph* graph_;
		QCPAxisRect* rect_;
		QCPDataMap backBuffer;
		int backBufferPosition = 0;
		QSettings settings;
		QVector<double> ticks;
		QVector<QString> tickLabels;
		QPen pen;
		QPen selectedPen;
		double mean;

		inline int getPlotBlockSize() const {
			return ( (settings.value("display/refresh").toDouble() / 1000) *
					settings.value("data/sample-rate").toDouble());
		}
};

}; // end subplot namespace
}; // end meaview namespace

#endif

