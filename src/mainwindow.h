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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiArea>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <QAction>
#include <QMessageBox>
#include <QCloseEvent>
#include <QIcon>
#include <QFont> // Required for QFont
#include <QColor> // Required for QColor
#include <QColorDialog> // Required for QColorDialog
#include <QDockWidget>
#include <QTextEdit>

#include "mditabbeddocumentarea.h" // Include our custom MDI sub-window
#include "documentwidget.h" // Required to access DocumentWidget
#include "pagegraphicsscene.h" // Required to access PageGraphicsScene
#include "pagetextitem.h" // Required to access PageTextItem
#include "keycapobject.h"

// MainWindow class is the main application window.
// It contains the QMdiArea to manage multiple MdiTabbedDocumentArea instances.
class MainWindow : public QMainWindow {
    Q_OBJECT // Macro required for any class that defines signals or slots
    
public:
    // Constructor: Takes a parent widget.
    explicit MainWindow(QWidget *parent = nullptr);
    
    static MainWindow* instance() { return s_instance; }
    
    void setAllDocumentsClosed(bool b) { m_allDocumentsClosed = b; }
    bool showMathWarnings() const { return m_showMathWarnings; }
    bool showMathStructure() const { return m_showMathStructure; }
    
public slots:
    // Slot to update the UI (menu/toolbar actions) based on the active MDI sub-window.
    void updateUi();
    void copySelectedItems();
    void pasteItems();
    
protected:
    // Overrides the close event to handle unsaved documents in all MDI sub-windows.
    void closeEvent(QCloseEvent *event) override;
    
private slots:
    // File menu actions
    void newWindow();           // Creates a new MDI sub-window (partition)
    void newDocument();         // Creates a new document in the active MDI sub-window
    void openDocument();        // Opens a document in the active MDI sub-window
    void saveDocument();        // Saves the current document
    void saveDocumentAs();      // Saves the current document to a new file
    void exportAsPDF();         // Exports everything as PDF without asking for a filename
    void closeDocument();       // Closes the current document
    void closeWindow();         // Closes the current MDI sub-window
    void exitApplication();     // Exits the application
    
    // Window menu actions
    // void tileWindows();         // Tiles all MDI sub-windows
    // void cascadeWindows();      // Cascades all MDI sub-windows
    // void nextWindow();          // Activates the next MDI sub-window
    // void previousWindow();      // Activates the previous MDI sub-window
    
    // Help menu actions
    void about();               // Shows an about dialog
    
    // Text Formatting Actions
    void applyStandardFormat();
    void applyTitleFormat();
    void applySubtitleFormat();
    void applyHeading1Format();
    void applyHeading2Format();
    void applyHeading3Format();
    void applyHeading4Format();
    void applyHeading5Format();
    void changeFontColor();
    void changeBackgroundColor();
    
    // Warnings for PageMathItem objecst on/off
    void toggleWarningsDisplay(bool checked);
    void toggleStructureDisplay(bool checked);
    void toggleInfoDisplay(bool checked);
    
private:
    static MainWindow* s_instance;
    QMdiArea *m_mdiArea = nullptr; // The MDI area that manages sub-windows
    bool m_canClose = false;
    bool m_allDocumentsClosed = true;
    PageTextItem *m_focusedTextItem = nullptr;
    bool m_showMathWarnings = true;
    bool m_showMathStructure = false;
    QList<QGraphicsItem*> m_copiedItems;
    
    // Actions for menu and toolbar
    QAction *m_newWindowAct = nullptr;
    QAction *m_newDocumentAct = nullptr;
    QAction *m_openDocumentAct = nullptr;
    QAction *m_saveDocumentAct = nullptr;
    QAction *m_saveDocumentAsAct = nullptr;
    QAction *m_exportAsPDFAct = nullptr;
    QAction *m_closeDocumentAct = nullptr;
    QAction *m_closeWindowAct = nullptr;
    QAction *m_exitAct = nullptr;
    QAction *m_copyAct = nullptr;
    QAction *m_pasteAct = nullptr;
    QAction *m_tileAct = nullptr;
    QAction *m_toggleWarningsAct = nullptr;
    QAction *m_toggleStructureAct = nullptr;
    /*    QAction *m_cascadeAct;
    QAction *m_nextAct;
    QAction *m_previousAct;
*/
    QAction *m_aboutAct = nullptr;
    
    // New actions for text formatting
    QAction *m_formatStandardAct = nullptr;
    QAction *m_formatTitleAct = nullptr;
    QAction *m_formatSubtitleAct = nullptr;
    QAction *m_formatHeading1Act = nullptr;
    QAction *m_formatHeading2Act = nullptr;
    QAction *m_formatHeading3Act = nullptr;
    QAction *m_formatHeading4Act = nullptr;
    QAction *m_formatHeading5Act = nullptr;
    QAction *m_changeFontColorAct = nullptr;
    QAction *m_changeBackgroundColorAct = nullptr;
    QAction *m_toggleKeyInfoAct = nullptr;
    
    QDockWidget *m_keyInfoDock = nullptr;
    QTextEdit *m_keyInfoEdit = nullptr;
    KeyCapObject *m_keyCapObject = nullptr;
    
    // Helper functions for UI setup
    void createActions();
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void createKeyInfoBox();
    void insertKeyCap(QTextCursor &cursor, const QString &text);
    void setKeyInfoText();
    
    // The document has been changed, mark that in the active document
    void activeDocumentChanged();
        
    // Returns the currently active MdiTabbedDocumentArea sub-window.
    MdiTabbedDocumentArea* activeMdiTabbedArea() const;
    
    // Helpers to get the currently focused PageTextItem and PageMathItem
    PageTextItem* activeTextItem() const;
    PageMathItem* activeMathItem() const;
};

#endif // MAINWINDOW_H
