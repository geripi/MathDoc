/* Copyright (C) 2025, 2026 Gerald Pichler (gerald.pichler@chello.at)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QSplashScreen>
#include <QPixmap>
#include <QElapsedTimer> // Needed for QElapsedTimer
#include <QCoreApplication> // Needed for QCoreApplication::processEvents()
#include <QScreen> // Needed for QScreen
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    // Create a QApplication instance. This is required for any Qt GUI application.
    QApplication app(argc, argv);
    
    
    int screenHeight;
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) { screenHeight = 1080; }
    else { screenHeight = screen->availableGeometry().height(); }
    int targetSplashHeight = screenHeight / 3;
    // 1. Load your splash image and scale it
    QPixmap originalPixmap(":icons/mathdoc_splash.png"); // Assuming you have a resource file with "splash.png"
    QPixmap pixmap = originalPixmap.scaledToHeight(targetSplashHeight, Qt::SmoothTransformation);
    
    // 2. Create a QSplashScreen instance
    QSplashScreen splash(pixmap);
    // 3. Show the splash screen
    splash.show();
    
    // 4. Process events to ensure the splash screen is painted immediately
    //    This is crucial, especially for longer initialization tasks.
    app.processEvents();
    // Use QElapsedTimer for a more robust delay that allows UI updates
    QElapsedTimer timer;
    timer.start();
    const int delayMilliseconds = 2000; // 2 seconds
/*    
    while (timer.elapsed() < delayMilliseconds) {
        app.processEvents();
    }
*/    
    app.setApplicationName("MathDoc");
    app.setApplicationVersion("1.0");
    app.setWindowIcon(QIcon(":/icons/mathdoc.png"));
    
    // Create an instance of our custom MainWindow class.
    MainWindow mainWin;
    mainWin.showMaximized(); // Start maximized for a better user experience
    
    splash.finish(&mainWin);
    
    // Start the Qt event loop. This makes the application responsive to user input.
    return app.exec();
}
