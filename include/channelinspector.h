/*! \file channelinspector.h
 *
 * Header file describing a class for inspecting in detail the data for a
 * particular channel.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#ifndef _MEAVIEW_CHANNEL_INSPECTOR_H_
#define _MEAVIEW_CHANNEL_INSPECTOR_H_

#include "settings.h"
#include "qcustomplot.h"

#include <QGridLayout>
#include <QWidget>
#include <QSettings>
#include <QCloseEvent>
#include <QRect>

namespace meaview {
namespace channelinspector {

/*! \class ChannelInspector
 *
 * The ChannelInspector class provides a standalone window display data
 * from a single channel, allowing higer-resolution inspection of the 
 * data from that channel. This is useful for data from an intracellular
 * electrode, where the values of the data may vary widely over time,
 * and yet small fluctuations may be relevant.
 */
class ChannelInspector : public QWidget {
	Q_OBJECT

	public:
		/*! Construct an inspector.
		 *
		 * \param parentPlot The main QCustomPlot object from which data will be copied.
		 * \param sourceGraph The line graph from which data will be copied.
		 * \param channel The channel number for this inspector.
		 * \param label The label for this channel. Often but not always the channel number.
		 * \param parent Parent widget.
		 */
		ChannelInspector(QCustomPlot* parentPlot, QCPGraph* sourceGraph,
				int channel, const QString& label, QWidget* parent = 0);
		
		/*! Destroy an inspector. */
		~ChannelInspector();

		/*! Return the channel number being inspected. */
		int channel();

		/*! Assign a new source graph to the inspector, for example, if subplots
		 * are rearranged when a new view is selected.
		 *
		 * \param g The new source graph object.
		 */
		void updateSourceGraph(QCPGraph *g);

		/*! Return the full position of the inspector window, prior to 
		 * minifying the MeaView application
		 */
		const QRect& fullPosition() const;

		/*! Request that the inspector save its current position before
		 * minifying, so that it can be returned to later.
		 */
		void saveFullPosition();

	signals:
		/*! Emitted just before the window closes, which allows the plot window
		 * to delete the inspector from its list.
		 *
		 * \param channel The data channel number being inspected. This allows the
		 * 	parent plot to know which inspector to delete.
		 */
		void aboutToClose(int channel);

	public slots:

		/*! Replot this inspector, drawing its data from the source graph.
		 *
		 * The ChannelInspector simply copies the data from the source graph.
		 */
		void replot();

	private:

		/* Handler function for a close event, which causes emission of the 
		 * aboutToClose signal. This is used to notify the parent plot to
		 * delete this inspector from its list.
		 */
		void closeEvent(QCloseEvent* event);

		/*! The window's main layout. */
		QGridLayout* m_layout;

		/*! The plot object holding all graphics objects. */
		QCustomPlot* m_plot;

		/*! This plot's axis, in which the data is plotted. */
		QCPAxisRect* m_rect;

		/*! This plot's graph, which manages the data. */
		QCPGraph* m_graph;

		/*! This plot's source graph, from which the data is copied. */
		QCPGraph* m_sourceGraph;

		/*! The channel number associated with this inspector. */
		int m_channel;

		/*! Global settings */
		QSettings m_settings;

		/*! Vector of ticks, upper/lower range and 0 */
		QVector<double> m_ticks;

		/*! Vector of tick labels. */
		QVector<QString> m_tickLabels;

		/*! Window rect before minifying, used to return to original
		 * positions.
		 */
		QRect m_fullPos;

}; // end ChannelInspector class
}; // end channelinspector namespace
}; // end meaview namespace

#endif

