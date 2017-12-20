/*! \file main.cc
 *
 * Main entry point for the MeaView application. This application
 * allows users to visualize multi-electrode array data, either streaming
 * from one of several data servers or played back from an old recording
 * file saved to disk.
 *
 * See the README.md for details.
 *
 * (C) 2015 Benjamin Naecker bnaecker@stanford.edu
 */

#include "meaviewwindow.h"

#include <QApplication>

/*! \function main
 * The main entry point for the `meaview` application.
 */
int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setOrganizationName("baccuslab");
	app.setApplicationName("MeaView");
	meaview::meaviewwindow::MeaviewWindow win;
	win.show();
	return app.exec();
}
