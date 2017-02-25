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

class ChannelInspector : public QWidget {
	Q_OBJECT

	public:
		/*! Construct an inspector */
		ChannelInspector(QCustomPlot* parentPlot, QCPGraph* sourceGraph,
				int channel, const QString& label, QWidget* parent = 0);
		
		/*! Destroy and inspector */
		~ChannelInspector();

		/*! Return the channel number being inspected */
		int channel();

		/*! Assign a new source graph to the inspector, for example, if subplots
		 * are rearranged when a new view is selected 
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
		 */
		void aboutToClose(int channel);

	public slots:
		/*! Replot this inspector, drawing its data from the source graph */
		void replot();

	private:
		/* Handler function for a close event, which causes emission of the 
		 * aboutToClose signal
		 */
		void closeEvent(QCloseEvent* event);

		/*! The window's main layout */
		QGridLayout* layout;

		/*! The plot object holding all graphics objects */
		QCustomPlot* plot;

		/*! This plot's axis, in which the data is plotted */
		QCPAxisRect* rect;

		/*! This plot's graph, which manages the data */
		QCPGraph* graph;

		/*! This plot's source graph, from which the data is copied */
		QCPGraph* sourceGraph;

		/*! The channel number associated with this inspector */
		int channel_;

		/*! Global settings */
		QSettings settings;

		/*! Vector of ticks, upper/lower range and 0 */
		QVector<double> ticks;
		QVector<QString> tickLabels;

		/*! Window rect before minifying, used to return to original
		 * positions.
		 */
		QRect fullPos;

}; // end ChannelInspector class
}; // end channelinspector namespace
}; // end meaview namespace

#endif

