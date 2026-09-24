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

// mainwindow.cpp
#include "mainwindow.h"
#include "helpers.h"

#include <QApplication>
#include <QIcon>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer> // For QTimer::singleShot
#include <QColorDialog> // For QColorDialog
#include <QMimeData>
#include <QClipboard>
#include <QTextDocumentFragment>
#include <QTextCursor>
#include <QTextDocument>

// Initialize the static member variable outside the class definition
MainWindow* MainWindow::s_instance = nullptr;

// Constructor for MainWindow.
MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent) {
    // SET THE STATIC POINTER HERE
    s_instance = this;
    
    setWindowTitle("MathDoc");
    
    m_mdiArea = new QMdiArea(this);
    m_mdiArea->setViewMode(QMdiArea::SubWindowView); // This is already set for split-screen
    m_mdiArea->setTabsClosable(true);
    m_mdiArea->setTabsMovable(true);
    setCentralWidget(m_mdiArea);
    
    // Connect the QMdiArea's subWindowActivated signal to our updateUi slot.
    // This ensures menu/toolbar actions are enabled/disabled correctly when switching windows.
    connect(m_mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::updateUi);
    
    createActions();
    createMenus();
    createToolBar();
    createStatusBar();
    createKeyInfoBox();
    
    updateUi();
    newWindow(); // Create an initial new window/partition for the user
}

// Creates all QActions for menus and toolbars.
void MainWindow::createActions() {
    // File Menu Actions
    m_newWindowAct = new QAction(QIcon(":/icons/new_window.png"), tr("New &Window"), this);
    m_newWindowAct->setShortcut(QKeySequence::New); // Reusing New shortcut for new window/partition
    m_newWindowAct->setStatusTip(tr("Create a new document window/partition"));
    connect(m_newWindowAct, &QAction::triggered, this, &MainWindow::newWindow);
    
    m_newDocumentAct = new QAction(QIcon(":/icons/new_document.png"), tr("New &Document"), this);
    m_newDocumentAct->setShortcut(QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_N)); // Ctrl+Shift+N
    m_newDocumentAct->setStatusTip(tr("Create a new document in the current window"));
    connect(m_newDocumentAct, &QAction::triggered, this, &MainWindow::newDocument);
    
    m_openDocumentAct = new QAction(QIcon(":/icons/open.png"), tr("&Open..."), this);
    m_openDocumentAct->setShortcut(QKeySequence::Open);
    m_openDocumentAct->setStatusTip(tr("Open an existing document"));
    connect(m_openDocumentAct, &QAction::triggered, this, &MainWindow::openDocument);
    
    m_saveDocumentAct = new QAction(QIcon(":/icons/save.png"), tr("&Save"), this);
    m_saveDocumentAct->setShortcut(QKeySequence::Save);
    m_saveDocumentAct->setStatusTip(tr("Save the active document"));
    connect(m_saveDocumentAct, &QAction::triggered, this, &MainWindow::saveDocument);
    
    m_saveDocumentAsAct = new QAction(QIcon(":/icons/save_as.png"), tr("Save &As..."), this);
    m_saveDocumentAsAct->setShortcut(QKeySequence::SaveAs);
    m_saveDocumentAsAct->setStatusTip(tr("Save the active document with a new name"));
    connect(m_saveDocumentAsAct, &QAction::triggered, this, &MainWindow::saveDocumentAs);

    m_exportAsPDFAct = new QAction(QIcon(":/icons/toPDF.png"), tr("&Export PDF"), this);
    m_exportAsPDFAct->setStatusTip(tr("Save the active document as PDF with the same file name"));
    connect(m_exportAsPDFAct, &QAction::triggered, this, &MainWindow::exportAsPDF);
    
    m_closeDocumentAct = new QAction(QIcon(":/icons/close_document.png"), tr("Close &Document"), this);
    m_closeDocumentAct->setStatusTip(tr("Close the active document"));
    connect(m_closeDocumentAct, &QAction::triggered, this, &MainWindow::closeDocument);
    
    m_closeWindowAct = new QAction(QIcon(":/icons/close_window.png"), tr("Close &Window"), this);
    m_closeWindowAct->setStatusTip(tr("Close the active document window/partition"));
    connect(m_closeWindowAct, &QAction::triggered, this, &MainWindow::closeWindow);
    
    m_exitAct = new QAction(QIcon(":/icons/exit.png"), tr("E&xit"), this);
    m_exitAct->setShortcut(QKeySequence::Quit);
    m_exitAct->setStatusTip(tr("Exit the application"));
    connect(m_exitAct, &QAction::triggered, this, &MainWindow::exitApplication);

    m_copyAct = new QAction(QIcon(":/icons/copy.png"), tr("&Copy"), this);
    m_copyAct->setShortcuts(QKeySequence::Copy);
    m_copyAct->setStatusTip(tr("Copy the current selection to the clipboard"));
    connect(m_copyAct, &QAction::triggered, this, &MainWindow::copySelectedItems);

    m_pasteAct = new QAction(QIcon(":/icons/paste.png"), tr("&Paste"), this);
    m_pasteAct->setShortcuts(QKeySequence::Paste);
    m_pasteAct->setStatusTip(tr("Paste content from the clipboard"));
    connect(m_pasteAct, &QAction::triggered, this, &MainWindow::pasteItems);
    
    // Window Menu Actions
    m_tileAct = new QAction(QIcon(":/icons/tile.png"), tr("&Tile"), this);
    m_tileAct->setStatusTip(tr("Tile the sub-windows"));
    connect(m_tileAct, &QAction::triggered, m_mdiArea, &QMdiArea::tileSubWindows);
    
/*    m_cascadeAct = new QAction(QIcon(":/icons/cascade.png"), tr("&Cascade"), this);
    m_cascadeAct->setStatusTip(tr("Cascade the sub-windows"));
    connect(m_cascadeAct, &QAction::triggered, m_mdiArea, &QMdiArea::cascadeSubWindows);
    
    m_nextAct = new QAction(QIcon(":/icons/next.png"), tr("Ne&xt"), this);
    m_nextAct->setShortcut(QKeySequence::NextChild);
    m_nextAct->setStatusTip(tr("Move to the next sub-window"));
    connect(m_nextAct, &QAction::triggered, m_mdiArea, &QMdiArea::activateNextSubWindow);
    
    m_previousAct = new QAction(QIcon(":/icons/previous.png"), tr("Pre&vious"), this);
    m_previousAct->setShortcut(QKeySequence::PreviousChild);
    m_previousAct->setStatusTip(tr("Move to the previous sub-window"));
    connect(m_previousAct, &QAction::triggered, m_mdiArea, &QMdiArea::activatePreviousSubWindow);
*/    
    // Help Menu Actions
    m_aboutAct = new QAction(QIcon(":/icons/about.png"), tr("&About MathDoc"), this);
    m_aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(m_aboutAct, &QAction::triggered, this, &MainWindow::about);
    
    // Toggle Warnings Action
    m_toggleWarningsAct = new QAction(QIcon(":/icons/warnings.png"), tr("&Display Math Warnings"), this);
    m_toggleWarningsAct->setCheckable(true); // <--- IMPORTANT: Makes it a toggle button
    m_toggleWarningsAct->setChecked(m_showMathWarnings); // Set initial state
    m_toggleWarningsAct->setStatusTip(tr("Toggle the display of warning statuses for Math items."));
    connect(m_toggleWarningsAct, &QAction::triggered, this, &MainWindow::toggleWarningsDisplay);
    
    // Toggle Structure Action
    m_toggleStructureAct = new QAction(QIcon(":/icons/structure.png"), tr("&Display Math Warnings"), this);
    m_toggleStructureAct->setCheckable(true); // <--- IMPORTANT: Makes it a toggle button
    m_toggleStructureAct->setChecked(m_showMathStructure); // Set initial state
    m_toggleStructureAct->setStatusTip(tr("Toggle the display of warning statuses for Math items."));
    connect(m_toggleStructureAct, &QAction::triggered, this, &MainWindow::toggleStructureDisplay);
    
    // New Text Formatting Actions
    m_formatStandardAct = new QAction(tr("Standard"), this);
    m_formatStandardAct->setStatusTip(tr("Apply Standard Text Format (Arial, 12pt, Normal)"));
    connect(m_formatStandardAct, &QAction::triggered, this, &MainWindow::applyStandardFormat);
    
    m_formatTitleAct = new QAction(tr("Title"), this);
    m_formatTitleAct->setStatusTip(tr("Apply Title Format (Arial, 20pt, Bold)"));
    connect(m_formatTitleAct, &QAction::triggered, this, &MainWindow::applyTitleFormat);
    
    m_formatSubtitleAct = new QAction(tr("Subtitle"), this);
    m_formatSubtitleAct->setStatusTip(tr("Apply Subtitle Format (Arial, 16pt, Bold)"));
    connect(m_formatSubtitleAct, &QAction::triggered, this, &MainWindow::applySubtitleFormat);
    
    m_formatHeading1Act = new QAction(tr("Heading 1"), this);
    m_formatHeading1Act->setStatusTip(tr("Apply Heading 1 Format (Arial, 18pt, Bold)"));
    connect(m_formatHeading1Act, &QAction::triggered, this, &MainWindow::applyHeading1Format);
    
    m_formatHeading2Act = new QAction(tr("Heading 2"), this);
    m_formatHeading2Act->setStatusTip(tr("Apply Heading 2 Format (Arial, 16pt, Bold)"));
    connect(m_formatHeading2Act, &QAction::triggered, this, &MainWindow::applyHeading2Format);
    
    m_formatHeading3Act = new QAction(tr("Heading 3"), this);
    m_formatHeading3Act->setStatusTip(tr("Apply Heading 3 Format (Arial, 14pt, Bold)"));
    connect(m_formatHeading3Act, &QAction::triggered, this, &MainWindow::applyHeading3Format);
    
    m_formatHeading4Act = new QAction(tr("Heading 4"), this);
    m_formatHeading4Act->setStatusTip(tr("Apply Heading 4 Format (Arial, 12pt, Bold)"));
    connect(m_formatHeading4Act, &QAction::triggered, this, &MainWindow::applyHeading4Format);
    
    m_formatHeading5Act = new QAction(tr("Heading 5"), this);
    m_formatHeading5Act->setStatusTip(tr("Apply Heading 5 Format (Arial, 12pt, Italic)"));
    connect(m_formatHeading5Act, &QAction::triggered, this, &MainWindow::applyHeading5Format);
    
    m_changeFontColorAct = new QAction(QIcon(":/icons/fontcolor.png"), tr("Font Color..."), this);
    m_changeFontColorAct->setStatusTip(tr("Change the font color of the active text item"));
    connect(m_changeFontColorAct, &QAction::triggered, this, &MainWindow::changeFontColor);
    
    m_changeBackgroundColorAct = new QAction(QIcon(":/icons/backgroundcolor.png"), tr("Background Color..."), this);
    m_changeBackgroundColorAct->setStatusTip(tr("Change the background color of the active text item"));
    connect(m_changeBackgroundColorAct, &QAction::triggered, this, &MainWindow::changeBackgroundColor);
    
    m_toggleKeyInfoAct = new QAction(QIcon(":/icons/info.png"), tr("Key Info"), this);
    m_toggleKeyInfoAct->setCheckable(true);
    m_toggleKeyInfoAct->setChecked(true);
    m_toggleKeyInfoAct->setToolTip(tr("Show or hide key information box"));
    connect(m_toggleKeyInfoAct, &QAction::triggered, this, &MainWindow::toggleInfoDisplay);
    
}

// Creates the menu bar.
void MainWindow::createMenus() {
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_newWindowAct);
    fileMenu->addAction(m_newDocumentAct);
    fileMenu->addAction(m_openDocumentAct);
    fileMenu->addAction(m_saveDocumentAct);
    fileMenu->addAction(m_saveDocumentAsAct);
    fileMenu->addAction(m_exportAsPDFAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_closeDocumentAct);
    fileMenu->addAction(m_closeWindowAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAct);
    
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_copyAct);
    editMenu->addAction(m_pasteAct);
    
    QMenu *formatMenu = menuBar()->addMenu(tr("F&ormat"));
    formatMenu->addAction(m_toggleWarningsAct); 
    formatMenu->addAction(m_toggleStructureAct); 
    formatMenu->addAction(m_formatStandardAct);
    formatMenu->addAction(m_formatTitleAct);
    formatMenu->addAction(m_formatSubtitleAct);
    formatMenu->addSeparator();
    formatMenu->addAction(m_formatHeading1Act);
    formatMenu->addAction(m_formatHeading2Act);
    formatMenu->addAction(m_formatHeading3Act);
    formatMenu->addAction(m_formatHeading4Act);
    formatMenu->addAction(m_formatHeading5Act);
    formatMenu->addSeparator();
    formatMenu->addAction(m_changeFontColorAct);
    formatMenu->addAction(m_changeBackgroundColorAct);
    
    QMenu *windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->addAction(m_tileAct);
/*    windowMenu->addAction(m_cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(m_nextAct);
    windowMenu->addAction(m_previousAct);
*/    
    menuBar()->addSeparator(); // Separator for consistency
    
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_aboutAct);
    
}

// Creates the tool bar.
void MainWindow::createToolBar() {
    QScreen *screen = QGuiApplication::primaryScreen();
    int screenHeight = screen->availableGeometry().height();
    
    int iconPixels = static_cast<int>(screenHeight * 0.04);
    
    //int systemIconSize = QApplication::style()->pixelMetric(QStyle::PM_ToolBarIconSize);
    qDebug() << "Icon size: " << iconPixels;
    QSize defaultIconSize(iconPixels, iconPixels);
    //QSize defaultIconSize(32, 32);
    
    QToolBar *fileToolBar = addToolBar(tr("File"));
    fileToolBar->setIconSize(defaultIconSize);
    fileToolBar->addAction(m_newWindowAct);
    fileToolBar->addAction(m_newDocumentAct);
    fileToolBar->addAction(m_openDocumentAct);
    fileToolBar->addAction(m_saveDocumentAct);
    fileToolBar->addAction(m_saveDocumentAsAct);
    fileToolBar->addAction(m_exportAsPDFAct);
    fileToolBar->addAction(m_closeDocumentAct); // Added close document to toolbar
    fileToolBar->addAction(m_closeWindowAct); // Added close window to toolbar
    
    QToolBar *editToolBar = addToolBar(tr("Edit"));
    editToolBar->setIconSize(defaultIconSize);
    editToolBar->addAction(m_copyAct);
    editToolBar->addAction(m_pasteAct);
    
    editToolBar->addAction(m_toggleWarningsAct);
    editToolBar->addAction(m_toggleStructureAct);
    
    // --- START: MODIFIED Format Toolbar Creation ---
    QToolBar *formatToolBar = addToolBar(tr("Format"));
    formatToolBar->setIconSize(defaultIconSize);
    
    // 1. Create a QMenu to hold all the individual formatting actions.
    QMenu *formatListMenu = new QMenu(this);
    formatListMenu->addAction(m_formatStandardAct);
    formatListMenu->addAction(m_formatTitleAct);
    formatListMenu->addAction(m_formatSubtitleAct);
    formatListMenu->addSeparator();
    formatListMenu->addAction(m_formatHeading1Act);
    formatListMenu->addAction(m_formatHeading2Act);
    formatListMenu->addAction(m_formatHeading3Act);
    formatListMenu->addAction(m_formatHeading4Act);
    formatListMenu->addAction(m_formatHeading5Act);
    
    // >>> NEW CONNECTION: Capture the active text item right before the menu displays <<<
    connect(formatListMenu, &QMenu::aboutToShow, this, [this](){
            m_focusedTextItem = activeTextItem(); });
    
    // 2. Create a QToolButton. This is what will appear on the toolbar.
    QToolButton *formatToolButton = new QToolButton(this);
    formatToolButton->setText(tr("Format"));
    // You can set an icon if you have one, e.g., formatToolButton->setIcon(QIcon(":/icons/format_icon.png"));
    formatToolButton->setPopupMode(QToolButton::InstantPopup); // Show menu on press
    formatToolButton->setMenu(formatListMenu); // Assign the menu to the button
    
    // 3. Add the QToolButton to the toolbar.
    formatToolBar->addWidget(formatToolButton);
//    formatToolBar->addSeparator(); // Separator before color options
    // --- END: MODIFIED Format Toolbar Creation ---

    formatToolBar->addSeparator(); // Separator before color options
    formatToolBar->addAction(m_changeFontColorAct);
    formatToolBar->addAction(m_changeBackgroundColorAct);
    
    QToolBar *windowToolBar = addToolBar(tr("Window"));
    windowToolBar->setIconSize(defaultIconSize);
    windowToolBar->addAction(m_tileAct);
/*    windowToolBar->addAction(m_cascadeAct);
    windowToolBar->addAction(m_nextAct);
    windowToolBar->addAction(m_previousAct);
*/
    
    QToolBar *helpToolBar = addToolBar(tr("Help"));
    helpToolBar->setIconSize(defaultIconSize);
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    helpToolBar->addWidget(spacer);
    helpToolBar->addAction(m_toggleKeyInfoAct);
}

// Creates the status bar.
void MainWindow::createStatusBar() {
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::createKeyInfoBox() {
    m_keyInfoDock = new QDockWidget(tr("key information"), this);
    
    // Prevent the user from moving it to another dock area if desired.
    m_keyInfoDock->setFeatures(
        QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetMovable
    );
    
    m_keyInfoEdit = new QTextEdit(m_keyInfoDock);
    m_keyCapObject = new KeyCapObject(m_keyInfoEdit);
    m_keyInfoEdit->document()->documentLayout()->registerHandler(KeyCapObject::Type, m_keyCapObject);
    
    setKeyInfoText();
    // User can select/copy the text, but cannot modify it.
    m_keyInfoEdit->setReadOnly(true);
    
    // Optional: don't allow the widget to become ridiculously narrow.
    m_keyInfoEdit->setMinimumWidth(50);
    
    m_keyInfoDock->setWidget(m_keyInfoEdit);
    
    // Put it on the right side of the main window.
    addDockWidget(Qt::RightDockWidgetArea, m_keyInfoDock);
}

void MainWindow::insertKeyCap(QTextCursor &cursor, const QString &text) {
    QTextCharFormat format;
    
    format.setObjectType(KeyCapObject::Type);
    format.setProperty(QTextFormat::UserProperty,text);
    
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
}

void MainWindow::setKeyInfoText() {
    QTextDocument *document = m_keyInfoEdit->document();
    
    document->clear();
    
    QTextCursor cursor(document);
    
    insertKeyCap(cursor, "ctrl");
    cursor.insertText(" + ");
    insertKeyCap(cursor, "T");
    cursor.insertText(" ... Textbox\n");
    
    insertKeyCap(cursor, "ctrl");
    cursor.insertText(" + ");
    insertKeyCap(cursor, "M");
    cursor.insertText(" ... Formula\n");
    
    insertKeyCap(cursor, "ctrl");
    cursor.insertText(" + ");
    insertKeyCap(cursor, "D");
    cursor.insertText(" ... Diagram\n");
    
    insertKeyCap(cursor, "ctrl");
    cursor.insertText(" + ");
    insertKeyCap(cursor, "I");
    cursor.insertText(" ... Image\n");
    
    
    cursor.insertText("\nInside a formula:\n");
    insertKeyCap(cursor, ":");
    cursor.insertText(" ... define new variable\n");
    
    insertKeyCap(cursor, "=");
    cursor.insertText(" ... calculate value\n");
    
    insertKeyCap(cursor, "ctrl"); cursor.insertText(" + ");
    insertKeyCap(cursor, "⇧"); cursor.insertText(" + ");  insertKeyCap(cursor, "=");
    cursor.insertText(" ... ≠ (not equal)\n");
    
    insertKeyCap(cursor, "alt"); cursor.insertText(" + ");
    insertKeyCap(cursor, "⇧"); cursor.insertText(" + ");  insertKeyCap(cursor, "=");
    cursor.insertText(" ... ＝ (equal)\n");
    
    insertKeyCap(cursor, "alt"); cursor.insertText(" + "); insertKeyCap(cursor, "<");
    cursor.insertText(" ... ≤\n");
    
    insertKeyCap(cursor, "alt"); cursor.insertText(" + ");
    insertKeyCap(cursor, "⇧"); cursor.insertText(" + "); insertKeyCap(cursor, ">");
    cursor.insertText(" ... ≥\n");
    
    cursor.insertText("\nGreek letters inside a formula:\n");
    
    cursor.insertText("    \\a = α\n");
    cursor.insertText("    \\b = β\n");
    cursor.insertText("    \\g = γ ...\n\n");
    
    cursor.insertText("Conditional definition:\n    ");
    
    insertKeyCap(cursor, "{");
    cursor.insertText(" ... new multiline def.\n");
    
    cursor.insertText("    ");
    
    insertKeyCap(cursor, "↵");
    cursor.insertText(" ... new line\n");
    
    cursor.insertText("    ");
    
    insertKeyCap(cursor, "⇧");
    cursor.insertText(" + ");
    insertKeyCap(cursor, "del");
    cursor.insertText(" ... delete line\n");
}

// Returns the currently active MdiTabbedDocumentArea sub-window.
MdiTabbedDocumentArea* MainWindow::activeMdiTabbedArea() const {
    if (QMdiSubWindow *activeSubWindow = m_mdiArea->activeSubWindow()) {
        return qobject_cast<MdiTabbedDocumentArea*>(activeSubWindow);
    }
    return nullptr;
}

// Helper to get the currently focused PageTextItem
PageTextItem* MainWindow::activeTextItem() const {
    MdiTabbedDocumentArea *activeMdi = activeMdiTabbedArea();
    if (!activeMdi) return nullptr;
    
    DocumentWidget *currentDoc = activeMdi->currentDocument();
    if (!currentDoc) return nullptr;
    
    PageGraphicsScene *scene = qobject_cast<PageGraphicsScene*>(currentDoc->graphicsView()->scene());
    if (!scene) return nullptr;
    
    // Check if the focused item is a PageTextItem
    return qgraphicsitem_cast<PageTextItem*>(scene->focusItem());
}

// Helper to get the currently focused PageMathItem
PageMathItem* MainWindow::activeMathItem() const {
    MdiTabbedDocumentArea *activeMdi = activeMdiTabbedArea();
    if (!activeMdi) return nullptr;
    
    DocumentWidget *currentDoc = activeMdi->currentDocument();
    if (!currentDoc) return nullptr;
    
    PageGraphicsScene *scene = qobject_cast<PageGraphicsScene*>(currentDoc->graphicsView()->scene());
    if (!scene) return nullptr;
    
    // Check if the focused item is a PageTextItem
    return qgraphicsitem_cast<PageMathItem*>(scene->focusItem());
}

// File menu slots
void MainWindow::newWindow() {
    MdiTabbedDocumentArea *newMdiWindow = new MdiTabbedDocumentArea(this);
    m_mdiArea->addSubWindow(newMdiWindow);
    newMdiWindow->showNormal(); // Changed to showNormal() for split-screen
    newMdiWindow->newDocument(); // Create a default new document within the new window
    
    // Connect the new signal to updateUi
    connect(newMdiWindow, &MdiTabbedDocumentArea::documentModificationStatusChanged,
            this, &MainWindow::updateUi);
    
    m_mdiArea->tileSubWindows(); // Tile all sub-windows to arrange them
    updateUi(); // Update UI after creating a new window
    //qDebug() << "MainWindow: New MDI window created and tiled.";
}

void MainWindow::newDocument() {
    if (MdiTabbedDocumentArea *activeMdi = activeMdiTabbedArea()) {
        activeMdi->newDocument();
    } else {
        newWindow(); // If no active window, create a new window and then a new document
        // The newWindow() call now handles newDocument() and tiling.
    }
    updateUi();
    //qDebug() << "MainWindow: New document action triggered.";
}

void MainWindow::openDocument() {
    if (MdiTabbedDocumentArea *activeMdi = activeMdiTabbedArea()) {
        activeMdi->openDocument();
    } else {
        MdiTabbedDocumentArea *newMdi = new MdiTabbedDocumentArea(this);
        m_mdiArea->addSubWindow(newMdi);
        newMdi->showNormal(); // Changed to showNormal() for split-screen
        newMdi->openDocument(); // Open document in the new window
        
        // Connect the new signal to updateUi for newly opened window
        connect(newMdi, &MdiTabbedDocumentArea::documentModificationStatusChanged,
                this, &MainWindow::updateUi);
        
        m_mdiArea->tileSubWindows(); // Tile all sub-windows after opening a document in a new window
    }
    updateUi();
    //qDebug() << "MainWindow: Open document action triggered.";
}

// Save document slot
void MainWindow::saveDocument() {
    if (MdiTabbedDocumentArea *activeMdi = activeMdiTabbedArea()) {
        if (activeMdi->saveCurrentDocument()) {
            statusBar()->showMessage(tr("Document saved"), 2000);
            //qDebug() << "MainWindow: Document saved successfully.";
        } else {
            statusBar()->showMessage(tr("Document save cancelled or failed"), 2000);
            //qDebug() << "MainWindow: Document save cancelled or failed.";
        }
    } else {
        statusBar()->showMessage(tr("No active document to save"), 2000);
        //qDebug() << "MainWindow: No active MDI window for save action.";
    }
    updateUi();
}

// Save As document slot
void MainWindow::saveDocumentAs() {
    if (MdiTabbedDocumentArea *activeMdi = activeMdiTabbedArea()) {
        if (activeMdi->saveCurrentDocumentAs()) {
            statusBar()->showMessage(tr("Document saved as new file"), 2000);
            //qDebug() << "MainWindow: Document saved as new file successfully.";
        } else {
            statusBar()->showMessage(tr("Save As cancelled or failed"), 2000);
            //qDebug() << "MainWindow: Save As cancelled or failed.";
        }
    } else {
        statusBar()->showMessage(tr("No active document to save"), 2000);
        //qDebug() << "MainWindow: No active MDI window for save as action.";
    }
    updateUi();
}

void MainWindow::exportAsPDF() {
    if (MdiTabbedDocumentArea *activeMdi = activeMdiTabbedArea()) {
        if (activeMdi->exportCurrentDocumentAsPDF()) {
            statusBar()->showMessage(tr("Document exported as PDF"), 2000);
            //qDebug() << "MainWindow: Document saved as new file successfully.";
        } else {
            statusBar()->showMessage(tr("export cancelled or failed"), 2000);
            //qDebug() << "MainWindow: Save As cancelled or failed.";
        }
    } else {
        statusBar()->showMessage(tr("No active document to export"), 2000);
        //qDebug() << "MainWindow: No active MDI window for save as action.";
    }
    
}

void MainWindow::closeDocument() {
    if (MdiTabbedDocumentArea *activeMdi = activeMdiTabbedArea()) {
        activeMdi->closeCurrentDocument();
    } else {
        statusBar()->showMessage(tr("No active document to close"), 2000);
    }
    updateUi();
    //qDebug() << "MainWindow: Close document action triggered.";
}

void MainWindow::closeWindow() {
    if (QMdiSubWindow *activeSubWindow = m_mdiArea->activeSubWindow()) {
        activeSubWindow->close(); // Request the sub-window to close itself
        //qDebug() << "MainWindow: Close window action triggered.";
    } else {
        statusBar()->showMessage(tr("No active window to close"), 2000);
        //qDebug() << "MainWindow: No active MDI sub-window for close window action.";
    }
    updateUi();
}

// Handles application exit, ensuring all documents are saved/discarded.
void MainWindow::exitApplication() {
    //qDebug() << "MainWindow: Exit application triggered.";
    // The closeEvent will handle prompting for saving all documents across all sub-windows.
    // If closeEvent accepts, the application will quit.
    close();
}

void MainWindow::about() {
    QMessageBox::about(this, tr("About MathDoc"),
                       tr("<b>MathDoc</b> is an application "
                       "developed with Qt6 and C++.<br>"
                       "Version %1").arg(QCoreApplication::applicationVersion()));
    //qDebug() << "MainWindow: About dialog shown.";
}

// The document has been changed, mark that in the active document
void MainWindow::activeDocumentChanged() {
    // The document has been changed, mark that in the active document
    MdiTabbedDocumentArea* activeMdi = activeMdiTabbedArea();
    if (activeMdi) {
        DocumentWidget* activeDoc = activeMdi->currentDocument();
        if (activeDoc) {
            activeDoc->setModified(true); emit activeDoc->contentModified();
        }
    }
}

// Text Formatting Slots
void MainWindow::applyStandardFormat() {
    if (m_focusedTextItem) {
        QFont font("Arial", 12, QFont::Normal);
        m_focusedTextItem->setFont(font);
        m_focusedTextItem->setBackGroundColor(Qt::transparent); // Clear background
        m_focusedTextItem->setDefaultTextColor(Qt::black); // Default text color
        activeDocumentChanged();
        //qDebug() << "Applied Standard format.";
        m_focusedTextItem->setFocus(Qt::ActiveWindowFocusReason);
        m_focusedTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    }
    updateUi();
    m_focusedTextItem = nullptr;
}

void MainWindow::applyTitleFormat() {
    if (m_focusedTextItem) {
        QFont font("Arial", 20, QFont::Bold);
        m_focusedTextItem->setFont(font);
        activeDocumentChanged();
        //qDebug() << "Applied Title format.";
        m_focusedTextItem->setFocus(Qt::ActiveWindowFocusReason);
        m_focusedTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    }
    updateUi();
    m_focusedTextItem = nullptr;
}

void MainWindow::applySubtitleFormat() {
    if (m_focusedTextItem) {
        QFont font("Arial", 16, QFont::Bold);
        m_focusedTextItem->setFont(font);
        activeDocumentChanged();
        //qDebug() << "Applied Subtitle format.";
        m_focusedTextItem->setFocus(Qt::ActiveWindowFocusReason);
        m_focusedTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    }
    updateUi();
    m_focusedTextItem = nullptr;
}

void MainWindow::applyHeading1Format() {
    if (m_focusedTextItem) {
        QFont font("Arial", 18, QFont::Bold);
        m_focusedTextItem->setFont(font);
        activeDocumentChanged();
        //qDebug() << "Applied Heading 1 format.";
        m_focusedTextItem->setFocus(Qt::ActiveWindowFocusReason);
        m_focusedTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    }
    updateUi();
    m_focusedTextItem = nullptr;
}

void MainWindow::applyHeading2Format() {
    if (m_focusedTextItem) {
        QFont font("Arial", 16, QFont::Bold);
        m_focusedTextItem->setFont(font);
        activeDocumentChanged();
        //qDebug() << "Applied Heading 2 format.";
        m_focusedTextItem->setFocus(Qt::ActiveWindowFocusReason);
        m_focusedTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    }
    updateUi();
    m_focusedTextItem = nullptr;
}

void MainWindow::applyHeading3Format() {
    if (m_focusedTextItem) {
        QFont font("Arial", 14, QFont::Bold);
        m_focusedTextItem->setFont(font);
        activeDocumentChanged();
        //qDebug() << "Applied Heading 3 format.";
        m_focusedTextItem->setFocus(Qt::ActiveWindowFocusReason);
        m_focusedTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    }
    updateUi();
    m_focusedTextItem = nullptr;
}

void MainWindow::applyHeading4Format() {
    if (m_focusedTextItem) {
        QFont font("Arial", 12, QFont::Bold);
        m_focusedTextItem->setFont(font);
        activeDocumentChanged();
        //qDebug() << "Applied Heading 4 format.";
        m_focusedTextItem->setFocus(Qt::ActiveWindowFocusReason);
        m_focusedTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    }
    updateUi();
    m_focusedTextItem = nullptr;
}

void MainWindow::applyHeading5Format() {
    if (m_focusedTextItem) {
        QFont font("Arial", 12, QFont::Normal, true); // Italic = true
        m_focusedTextItem->setFont(font);
        activeDocumentChanged();
        //qDebug() << "Applied Heading 5 format.";
        m_focusedTextItem->setFocus(Qt::ActiveWindowFocusReason);
        m_focusedTextItem->setTextInteractionFlags(Qt::TextEditorInteraction);
    }
    updateUi();
    m_focusedTextItem = nullptr;
}

void MainWindow::changeFontColor() {
    if (PageTextItem *item = activeTextItem()) {
        QColor initialColor = item->defaultTextColor();
        QTextCursor originalCursor = item->textCursor();
        QColor color = QColorDialog::getColor(initialColor, this, tr("Select Font Color"));
        if (color.isValid()) {
            item->setDefaultTextColor(color);
            //qDebug() << "Changed font color to" << color;
        }
        item->setTextInteractionFlags(Qt::TextEditorInteraction);
        item->setFocus();
        item->setTextCursor(originalCursor);
        activeDocumentChanged();
    }
    updateUi();
}

void MainWindow::changeBackgroundColor() {
    if (PageTextItem *item = activeTextItem()) {
        // Get the current background color from the text document
        QColor initialColor = item->backGroundColor();
        QTextCursor originalCursor = item->textCursor();
        QColor color = QColorDialog::getColor(initialColor, this, tr("Select Background Color"));
        if (color.isValid()) {
            item->setBackGroundColor(color);
            //qDebug() << "Changed background color to" << color;
        }
        item->setTextInteractionFlags(Qt::TextEditorInteraction);
        item->setFocus();
        item->setTextCursor(originalCursor);
        activeDocumentChanged();
    }
    updateUi();
}

// Slot to handle the "show warning" toggle button state
void MainWindow::toggleWarningsDisplay(bool checked) {
    m_showMathWarnings = checked;
    qDebug() << "MainWindow: Math Warnings Display Toggled. State:" << m_showMathWarnings;
    // Iterate over all sub-windows in the MDI area
    for (QMdiSubWindow *subWindow : m_mdiArea->subWindowList()) {
        // Safely cast the sub-window to our custom MdiTabbedDocumentArea
        MdiTabbedDocumentArea *mdiArea = qobject_cast<MdiTabbedDocumentArea*>(subWindow);
        if (mdiArea) {
            if (DocumentWidget *doc = mdiArea->currentDocument()) {
                // Option 1: Request a repaint of the graphics view containing the scene.
                doc->graphicsView()->viewport()->update();
            }
        }
    }
    statusBar()->showMessage(m_showMathWarnings ? tr("Math warnings are now displayed") : tr("Math warnings are now hidden"), 3000);
}

// Slot to handle the "show structure" toggle button state
void MainWindow::toggleStructureDisplay(bool checked) {
    m_showMathStructure = checked;
    qDebug() << "MainWindow: Math Warnings Display Toggled. State:" << m_showMathWarnings;
    // Iterate over all sub-windows in the MDI area
    for (QMdiSubWindow *subWindow : m_mdiArea->subWindowList()) {
        // Safely cast the sub-window to our custom MdiTabbedDocumentArea
        MdiTabbedDocumentArea *mdiArea = qobject_cast<MdiTabbedDocumentArea*>(subWindow);
        if (mdiArea) {
            if (DocumentWidget *doc = mdiArea->currentDocument()) {
                // Option 1: Request a repaint of the graphics view containing the scene.
                doc->graphicsView()->viewport()->update();
            }
        }
    }
    statusBar()->showMessage(m_showMathWarnings ? tr("Math warnings are now displayed") : tr("Math warnings are now hidden"), 3000);
}

// Slot to handle the "show info" toggle button state
void MainWindow::toggleInfoDisplay(bool checked) {
    m_keyInfoDock->setVisible(checked);
}

// Updates the enabled/disabled state of UI actions.
void MainWindow::updateUi() {
    MdiTabbedDocumentArea *activeMdi = activeMdiTabbedArea();
    bool hasActiveMdi = (activeMdi != nullptr);
    bool hasActiveDocument = (hasActiveMdi && activeMdi->currentDocument() != nullptr);
    bool isDocumentModified = (hasActiveDocument && activeMdi->currentDocument()->isModified());
    
    m_newDocumentAct->setEnabled(hasActiveMdi);
    m_openDocumentAct->setEnabled(true); // Always allow opening a document
    m_saveDocumentAct->setEnabled(hasActiveDocument && isDocumentModified); // Only enable if document is active AND modified
    m_saveDocumentAsAct->setEnabled(hasActiveDocument); // Enable if any document is active
    m_closeDocumentAct->setEnabled(hasActiveDocument);
    m_closeWindowAct->setEnabled(hasActiveMdi);
    
    // Enable Tile/Cascade only if there's more than one sub-window
    m_tileAct->setEnabled(m_mdiArea->subWindowList().count() > 1);
/*    m_cascadeAct->setEnabled(m_mdiArea->subWindowList().count() > 1);
    m_nextAct->setEnabled(hasActiveMdi);
    m_previousAct->setEnabled(hasActiveMdi);
*/    
    // Enable formatting actions only if a PageTextItem is active and has focus
    PageTextItem *focusedTextItem = activeTextItem();
    bool isTextItemFocused = (focusedTextItem != nullptr && focusedTextItem->textInteractionFlags() == Qt::TextEditorInteraction);
    //qDebug() << "MainWindow::updateUi: isTextItemFocused = " << isTextItemFocused;
    
    m_formatStandardAct->setEnabled(isTextItemFocused);
    m_formatTitleAct->setEnabled(isTextItemFocused);
    m_formatSubtitleAct->setEnabled(isTextItemFocused);
    m_formatHeading1Act->setEnabled(isTextItemFocused);
    m_formatHeading2Act->setEnabled(isTextItemFocused);
    m_formatHeading3Act->setEnabled(isTextItemFocused);
    m_formatHeading4Act->setEnabled(isTextItemFocused);
    m_formatHeading5Act->setEnabled(isTextItemFocused);
    m_changeFontColorAct->setEnabled(isTextItemFocused);
    m_changeBackgroundColorAct->setEnabled(isTextItemFocused);
    
    // Update status bar message when UI updates (e.g. after a document is saved)
    if (hasActiveDocument && !isDocumentModified) {
        statusBar()->showMessage(tr("Document saved or unchanged"), 2000);
    } else if (hasActiveDocument && isDocumentModified) {
        statusBar()->showMessage(tr("Document modified (unsaved)"), 2000);
    } else {
        statusBar()->showMessage(tr("Ready"), 2000);
    }
    //qDebug() << "MainWindow: UI updated. Has MDI:" << hasActiveMdi << "Has Doc:" << hasActiveDocument << "Modified:" << isDocumentModified << "Text Item Focused:" << isTextItemFocused;
}

// Override closeEvent to handle unsaved documents in all MDI sub-windows.
void MainWindow::closeEvent(QCloseEvent *event) {
    qDebug() << "MainWindow closeEvent: Received close event.";
    m_allDocumentsClosed = true;
    // Request all sub-windows to close.
    m_mdiArea->closeAllSubWindows();
    
    if (m_allDocumentsClosed) {
        qDebug() << "Deferred check: All sub-windows gone, application should quit naturally.";
        m_canClose = true;
        event->accept();
    } else {
        qDebug() << "Deferred check: Some sub-windows still remain. Application will not quit automatically.";
        m_canClose = false;
        event->ignore();
    }

}

void MainWindow::copySelectedItems() {
    DocumentWidget* activeDocument = m_mdiArea->activeSubWindow() ?
    qobject_cast<MdiTabbedDocumentArea*>(m_mdiArea->activeSubWindow())->currentDocument() : nullptr;
    
    if (!activeDocument) return;
    
    QGraphicsScene* scene = activeDocument->graphicsView()->scene();
    m_copiedItems = scene->selectedItems();
    
}

void MainWindow::pasteItems() {
    DocumentWidget* activeDocument = m_mdiArea->activeSubWindow() ?
    qobject_cast<MdiTabbedDocumentArea*>(m_mdiArea->activeSubWindow())->currentDocument() : nullptr;
    
    if (!activeDocument) return;
    
    PageGraphicsScene* scene = dynamic_cast<PageGraphicsScene*>( activeDocument->graphicsView()->scene() );
    scene->insertFrames(m_copiedItems);
    scene->compute();
}
