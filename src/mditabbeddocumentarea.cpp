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

// mditabbeddocumentarea.cpp
#include "mditabbeddocumentarea.h"
#include <QVBoxLayout> // For arranging widgets vertically
#include <QDebug> // For debugging output
#include <QFileDialog> // For QFileDialog::getSaveFileName, QFileDialog::getOpenFileName
#include "mainwindow.h"

// Constructor for MdiTabbedDocumentArea.
MdiTabbedDocumentArea::MdiTabbedDocumentArea(MainWindow *mainWindow, QWidget *parent)
    : QMdiSubWindow(parent), m_MainWindow(mainWindow) {
    qDebug() << "MdiTabbedDocumentArea constructor:" << this; // Debug: Log creation
    // Create a QTabWidget.
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true); // Allow tabs to be closed by the user
    m_tabWidget->setMovable(true);      // Allow tabs to be reordered

    // Set the tab widget as the central widget of the QMdiSubWindow.
    setWidget(m_tabWidget);

    // Connect signals for tab changes and tab close requests.
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MdiTabbedDocumentArea::onCurrentTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MdiTabbedDocumentArea::onTabCloseRequested);

    setAttribute(Qt::WA_DeleteOnClose); // Ensure sub-window is deleted when closed
}

MdiTabbedDocumentArea::~MdiTabbedDocumentArea() {
    qDebug() << "MdiTabbedDocumentArea destructor:" << this;
}

// Creates a new, empty document and adds it as a new tab.
DocumentWidget* MdiTabbedDocumentArea::newDocument() {
    DocumentWidget *docWidget = new DocumentWidget(m_MainWindow,this);
    addDocumentTab(docWidget);
    docWidget->setModified(true); // New document is initially modified (unsaved)
    m_tabWidget->setCurrentWidget(docWidget);
    qDebug() << "MdiTabbedDocumentArea: Created new document.";
    return docWidget;
}

// Opens an existing document from a file and adds it as a new tab.
DocumentWidget* MdiTabbedDocumentArea::openDocument(const QString &filePath) {
    QString fileName = filePath;
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getOpenFileName(this, "Open Document",
                                                QString(), "MathDoc Files (*.mad);;All Files (*)");
    }

    if (fileName.isEmpty()) {
        qDebug() << "MdiTabbedDocumentArea: Open cancelled by user.";
        return nullptr;
    }

    // Check if the document is already open in any tab within this sub-window
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        DocumentWidget *existingDoc = qobject_cast<DocumentWidget*>(m_tabWidget->widget(i));
        if (existingDoc && QFileInfo(existingDoc->filePath()) == QFileInfo(fileName)) {
            m_tabWidget->setCurrentIndex(i);
            QMessageBox::information(this, "Document Already Open",
                                     QString("The document '%1' is already open in this window.")
                                         .arg(QFileInfo(fileName).fileName()));
            qDebug() << "MdiTabbedDocumentArea: Document already open:" << fileName;
            return existingDoc;
        }
    }

    DocumentWidget *docWidget = new DocumentWidget(m_MainWindow ,this, fileName);
    if (docWidget->load()) {
        addDocumentTab(docWidget);
        m_tabWidget->setCurrentWidget(docWidget);
        qDebug() << "MdiTabbedDocumentArea: Opened document:" << fileName;
        return docWidget;
    } else {
        delete docWidget; // Delete the widget if loading failed
        qDebug() << "MdiTabbedDocumentArea: Failed to open document:" << fileName;
        return nullptr;
    }
}

// Saves the currently active document.
bool MdiTabbedDocumentArea::saveCurrentDocument() {
    DocumentWidget *currentDoc = currentDocument();
    if (!currentDoc) {
        qDebug() << "MdiTabbedDocumentArea save: No current document.";
        return false;
    }

    if (currentDoc->filePath().isEmpty()) {
        // If the document has no file path, treat it as Save As
        qDebug() << "MdiTabbedDocumentArea save: Document has no path, calling saveCurrentDocumentAs().";
        return saveCurrentDocumentAs();
    } else {
        qDebug() << "MdiTabbedDocumentArea save: Saving to existing path:" << currentDoc->filePath();
        bool saved = currentDoc->save();
        if (saved) {
            // Update the tab title to remove '*' if it was modified
            onCurrentTabChanged(m_tabWidget->currentIndex());
            emit documentModificationStatusChanged(); // Emit after saving
        }
        return saved;
    }
}

// Saves the currently active document to a new file path (Save As).
bool MdiTabbedDocumentArea::saveCurrentDocumentAs() {
    DocumentWidget *currentDoc = currentDocument();
    if (!currentDoc) {
        qDebug() << "MdiTabbedDocumentArea saveAs: No current document.";
        return false;
    }

    QString initialPath = QFileInfo(currentDoc->filePath()).fileName();
    if (initialPath.isEmpty()) {
        initialPath = "Untitled.mad"; // Default name for new documents
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Document As",
                                                    initialPath, "MathDoc Files (*.mad);;All Files (*)");
    if (fileName.isEmpty()) {
        qDebug() << "MdiTabbedDocumentArea saveAs: Save As cancelled by user.";
        return false;
    }

    // Ensure the file has a .mad extension if not provided
    if (!fileName.toLower().endsWith(".mad")) {
        fileName += ".mad";
    }

    currentDoc->setFilePath(fileName); // Update the document's file path
    qDebug() << "MdiTabbedDocumentArea saveAs: Saving to new path:" << fileName;
    bool saved = currentDoc->save();
    if (saved) {
        // Update the tab title to reflect the new file name and remove '*'
        onCurrentTabChanged(m_tabWidget->currentIndex());
        emit documentModificationStatusChanged(); // Emit after saving as
    }
    return saved;
}

bool MdiTabbedDocumentArea::exportCurrentDocumentAsPDF() {
    DocumentWidget *currentDoc = currentDocument();
    if (!currentDoc) {
        qDebug() << "MdiTabbedDocumentArea exportCurrentDocumentAsPDF: No current document.";
        return false;
    }
    
    QString initialPath = QFileInfo(currentDoc->filePath()).fileName();
    if (initialPath.isEmpty() || initialPath == "Untitled.mad") {
        qDebug() << "MdiTabbedDocumentArea exportCurrentDocumentAsPDF: Current document has no name yet.";
        return false;
    }
    
    QString pdfPath = currentDoc->filePath(); pdfPath.chop(4); pdfPath += QString(".pdf");
    qDebug() << "MdiTabbedDocumentArea exportCurrentDocumentAsPDF: pdfPath = " << pdfPath;
    
    return currentDoc->exportPDF(pdfPath);
}

// Closes the currently active document.
bool MdiTabbedDocumentArea::closeCurrentDocument() {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex == -1) {
        qDebug() << "MdiTabbedDocumentArea close: No document selected.";
        return true; // No document to close, consider it successful
    }

    DocumentWidget *docToClose = currentDocument();
    if (!docToClose) {
        qDebug() << "MdiTabbedDocumentArea close: Current widget is not a DocumentWidget.";
        return true;
    }

    if (maybeSaveDocument(docToClose)) {
        // If maybeSaveDocument returns true, it means user saved, discarded, or there were no changes.
        // So, it's safe to close the tab.
        m_tabWidget->removeTab(currentIndex);
        delete docToClose; // Delete the DocumentWidget
        qDebug() << "MdiTabbedDocumentArea close: Document closed and deleted from tab widget.";

        if (m_tabWidget->count() == 0) {
            // If the last document in this MDI sub-window is closed, close the sub-window itself.
            // This will trigger its closeEvent, which will also call maybeSaveDocument if needed.
            close(); // Close the QMdiSubWindow
            qDebug() << "MdiTabbedDocumentArea close: Last document closed, closing sub-window.";
        }
        emit documentModificationStatusChanged(); // Emit after closing a document (affects state)
        return true;
    }
    qDebug() << "MdiTabbedDocumentArea close: Document close cancelled by user.";
    return false; // User cancelled saving
}

// Returns the DocumentWidget currently displayed in the active tab.
DocumentWidget* MdiTabbedDocumentArea::currentDocument() const {
    return qobject_cast<DocumentWidget*>(m_tabWidget->currentWidget());
}

// Helper function to create a new tab for a DocumentWidget.
void MdiTabbedDocumentArea::addDocumentTab(DocumentWidget *docWidget) {
    QString title = docWidget->filePath().isEmpty() ? "Untitled" : QFileInfo(docWidget->filePath()).fileName();
    m_tabWidget->addTab(docWidget, title);

    // Connect contentModified signal to update the tab title
    connect(docWidget, &DocumentWidget::contentModified,
            this, &MdiTabbedDocumentArea::onDocumentContentModified);
    qDebug() << "MdiTabbedDocumentArea: Added document tab with title:" << title;
}

// Slot called when the current tab changes. Updates the sub-window title.
void MdiTabbedDocumentArea::onCurrentTabChanged(int index) {
    Q_UNUSED(index);
    DocumentWidget *doc = currentDocument();
    QString subWindowTitle = "New Window"; // Default for a new partition without a file
    if (doc) {
        QString docTitle = doc->filePath().isEmpty() ? "Untitled" : QFileInfo(doc->filePath()).fileName();
        if (doc->isModified()) {
            docTitle += "[*]"; // Add asterisk if modified
        }
        m_tabWidget->setTabText(m_tabWidget->currentIndex(), docTitle); // Update current tab title
        subWindowTitle = QString("Partition - %1").arg(docTitle); // Set sub-window title based on active doc
    } else {
        // If no document is active (e.g., after closing the last document in a window)
        subWindowTitle = "Partition - Empty";
    }
    setWindowTitle(subWindowTitle); // Update the sub-window's title
    emit documentModificationStatusChanged(); // Emit when current tab changes (affects state)
    qDebug() << "MdiTabbedDocumentArea: Current tab changed, sub-window title set to:" << subWindowTitle;
}

// Slot called when a tab is requested to be closed by the user (e.g., clicking 'x' on tab).
void MdiTabbedDocumentArea::onTabCloseRequested(int index) {
    qDebug() << "MdiTabbedDocumentArea: Tab close requested for index:" << index;
    DocumentWidget *docToClose = qobject_cast<DocumentWidget*>(m_tabWidget->widget(index));
    if (docToClose) {
        // Temporarily set this document as current to make maybeSaveDocument work correctly
        m_tabWidget->setCurrentIndex(index);
        if (maybeSaveDocument(docToClose)) {
            m_tabWidget->removeTab(index);
            delete docToClose; // Delete the DocumentWidget instance
            qDebug() << "MdiTabbedDocumentArea: Tab closed and document deleted.";
            if (m_tabWidget->count() == 0) {
                // If this was the last tab in this MDI sub-window, close the sub-window itself.
                close(); // Triggers closeEvent for QMdiSubWindow
                qDebug() << "MdiTabbedDocumentArea: Last tab closed, closing MDI sub-window.";
            }
            emit documentModificationStatusChanged(); // Emit after closing a tab
        } else {
            qDebug() << "MdiTabbedDocumentArea: Tab close cancelled by user.";
        }
    }
}

// Slot called when a document's content is modified. Updates the tab title.
void MdiTabbedDocumentArea::onDocumentContentModified() {
    DocumentWidget *doc = currentDocument();
    if (doc) {
        int index = m_tabWidget->indexOf(doc);
        if (index != -1) {
            QString docTitle = doc->filePath().isEmpty() ? "Untitled" : QFileInfo(doc->filePath()).fileName();
            if (doc->isModified()) {
                docTitle += "[*]";
            }
            m_tabWidget->setTabText(index, docTitle);
            setWindowTitle(QString("Partition - %1").arg(docTitle)); // Update sub-window title
            qDebug() << "MdiTabbedDocumentArea: Document content modified, tab title updated for index" << index;
            emit documentModificationStatusChanged(); // Emit this signal to notify MainWindow
        }
    }
}

// Helper function to prompt the user to save if a document is modified.
// Returns true if the user saved or chose not to save, false if they cancelled.
bool MdiTabbedDocumentArea::maybeSaveDocument(DocumentWidget *doc) {
    if (!doc || !doc->isModified()) {
        qDebug() << "maybeSaveDocument: Document not modified or null, returning true.";
        return true; // Document is not modified, no need to save
    }

    // Prompt the user to save.
    QMessageBox::StandardButton ret = QMessageBox::warning(this, "MathDoc",
                                                           QString("The document \"%1\" has been modified.\n"
                                                                   "Do you want to save your changes?")
                                                               .arg(QFileInfo(doc->filePath()).fileName().isEmpty() ? "Untitled" : QFileInfo(doc->filePath()).fileName()),
                                                           QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
        qDebug() << "maybeSaveDocument: User chose SAVE for" << (doc->filePath().isEmpty() ? "Untitled" : QFileInfo(doc->filePath()).fileName());
        // Call saveCurrentDocument() which handles both save and saveAs
        return saveCurrentDocument();
    } else if (ret == QMessageBox::Discard) {
        qDebug() << "maybeSaveDocument: User chose DISCARD for" << (doc->filePath().isEmpty() ? "Untitled" : QFileInfo(doc->filePath()).fileName());
        return true; // User chose to discard changes
    } else if (ret == QMessageBox::Cancel) {
        qDebug() << "maybeSaveDocument: User chose CANCEL for" << (doc->filePath().isEmpty() ? "Untitled" : QFileInfo(doc->filePath()).fileName());
        return false; // User cancelled
    }
    return true; // Should not be reached, but as a fallback
}

// Overrides the close event for the MdiTabbedDocumentArea.
void MdiTabbedDocumentArea::closeEvent(QCloseEvent *event) {
    qDebug() << "MdiTabbedDocumentArea closeEvent: Received close event for" << windowTitle();
    // Iterate through all tabs and check for unsaved documents
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        DocumentWidget *doc = qobject_cast<DocumentWidget*>(m_tabWidget->widget(i));
        if (doc && doc->isModified()) {
            m_tabWidget->setCurrentIndex(i); // Bring the modified tab to front
            if (!maybeSaveDocument(doc)) {
                event->ignore(); // User cancelled saving, so ignore the close event for the sub-window
                qDebug() << "MdiTabbedDocumentArea closeEvent: Close cancelled due to unsaved document.";
                m_MainWindow->setAllDocumentsClosed(false);
                return;
            }
        }
    }
    event->accept(); // All documents are saved or discarded, accept the close event
    qDebug() << "MdiTabbedDocumentArea closeEvent: All documents handled, accepting close.";
}
