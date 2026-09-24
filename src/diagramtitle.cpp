#include "diagramtitle.h"
#include "helpers.h"
#include "mainwindow.h"
#include "pagegraphicsscene.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTextLayout>
#include <QGraphicsSceneEvent>
#include <QGraphicsScene>


DiagramTitle::DiagramTitle(Data *d, PageDiagramItem *parent)
: QGraphicsObject(parent),
  m_data(d),
  m_parent(parent),
  m_font(m_fontName, static_cast<int>(m_mathFontSize), QFont::Bold),
  m_subFont(m_fontName, static_cast<int>(m_mathFontSize*m_subScriptScale), QFont::Bold),
  m_fontMetrics(m_font),
  m_subFontMetrics(m_subFont)
{
    setFlags(ItemIsFocusable);
    setAcceptHoverEvents(true);
    m_unit = nullptr;
    //updateBoundingRect();
}

DiagramTitle::DiagramTitle(const QString &text, Data *d, PageDiagramItem *parent)
: DiagramTitle(d, parent) {
    setText(text);
}

void DiagramTitle::initUnit(PageMathItem *u) {
    if (u) {
        m_unit = u;
    } else {
        m_unit = new PageMathItem(m_data, this);
    }
    m_unit->getMathLeaf()->setIsUsedAsUnit(true);
    m_unit->setIsInDiagramTitle(true);
    QKeyEvent event(QEvent::KeyPress, Qt::Key_ParenLeft, Qt::NoModifier, QStringLiteral("("));
    m_unit->getMathLeaf()->keyPressEvent(&event);
    
    PageGraphicsScene *pgs = qobject_cast<PageGraphicsScene*>(m_parent->parentObject());
    
    connect(m_unit, &PageMathItem::itemDataChanged, pgs, &PageGraphicsScene::documentContentItemsChanged);
    connect(m_unit, &PageMathItem::editingStarted, pgs, &PageGraphicsScene::onItemEditingStarted);
    connect(m_unit, &PageMathItem::editingFinished, pgs, &PageGraphicsScene::onItemEditingFinished);
    //connect(m_unit, &PageMathItem::itemSelected, pgs->getMainWindow(), &MainWindow::updateUi);
    connect(m_unit, &PageMathItem::computeWholeDocument, pgs, &PageGraphicsScene::compute);
    
    connect(&m_cursorTimer, &QTimer::timeout, this, &DiagramTitle::toggleCursor);

}

void DiagramTitle::setText(const QString &text) {
    if (m_text == text)
        return;
    
    m_text = text;
    //updateBoundingRect();
    //update();
}

QJsonObject DiagramTitle::toJson() const {
    QJsonObject object;
    object["text"] = m_text;
    object["unit"] = m_unit->toJson();
    
    return object;
}

void DiagramTitle::fromJson(const QJsonObject& object, Data *data, QGraphicsItem *parent) {
    m_text = object["text"].toString();
    
    QJsonObject mathEditObject = object["unit"].toObject();
    m_unit = PageMathItem::fromJson(mathEditObject, data, this);
    m_unit->getMathLeaf()->setIsUsedAsUnit(true);
    m_unit->setIsInDiagramTitle(true);
    
    PageGraphicsScene *pgs = qobject_cast<PageGraphicsScene*>(parent->parentObject());
    
    connect(m_unit, &PageMathItem::itemDataChanged, pgs, &PageGraphicsScene::documentContentItemsChanged);
    connect(m_unit, &PageMathItem::editingStarted, pgs, &PageGraphicsScene::onItemEditingStarted);
    connect(m_unit, &PageMathItem::editingFinished, pgs, &PageGraphicsScene::onItemEditingFinished);
    //connect(m_unit, &PageMathItem::itemSelected, pgs->getMainWindow(), &MainWindow::updateUi);
    connect(m_unit, &PageMathItem::computeWholeDocument, pgs, &PageGraphicsScene::compute);
    
}

void DiagramTitle::setMathFontSize(qreal s) {
    m_mathFontSize = s;
    m_font.setPointSizeF(s);
    m_subFont.setPointSizeF(s*m_subScriptScale);
    updateBoundingRect();
}

void DiagramTitle::updateBoundingRect() {
    QString contStr = m_text;
    QString normStr = contStr.section('_', 0, 0);
    QString subStr = contStr.section('_', 1, -1);
    //qDebug() << "DiagramTitle::updateBoundingRect: m_unit = " << m_unit;
    QRectF unitRect(0,0,1,1);
    if(m_unit)
        if(m_unit->getMathLeaf())
            unitRect = m_unit->getMathLeaf()->getBoundingRectangle();
    qreal width = textWidth(normStr) + subtextWidth(subStr) + textWidth(m_autoUnit) +
                  unitRect.width();
    qreal height = m_fontMetrics.height() + m_fontMetrics.descent();
    if (height < unitRect.height())
        height = unitRect.height();
    
    m_unit->setPos(textWidth(normStr) + subtextWidth(subStr) + textWidth(m_autoUnit) + textWidth(" "),
                   -0.35*m_fontMetrics.ascent()
                  );
    //m_unit->setPos(0.0, 0.0);
    
    qreal yPos = -m_fontMetrics.ascent();
    if (unitRect.y()-0.35*m_fontMetrics.ascent() < yPos)
        yPos = unitRect.y()-0.35*m_fontMetrics.ascent();
    
    QRectF rect = QRectF(0, yPos, width, height);
    
    prepareGeometryChange();
    m_boundingRect = rect;
}

void DiagramTitle::compute() {
    if (m_unit) {
        if (m_unit->getMathLeaf()) {
            m_unit->getMathLeaf()->setIsUsedAsUnit(true);
            m_unit->compute();
            m_unit->getMathLeaf()->setIsUsedAsUnit(true);
            
            MathVariable unitVar = m_unit->getMathLeaf()->getValue();
            QList<qreal> manualUnits = unitVar.unit();
            MathVariable dataVar; if (m_data->contains(m_text)) dataVar = m_data->getValue(m_text);
            QList<qreal> units = dataVar.unit();
            QList<qreal> unresolvedUnits = {units[0] - manualUnits[0],
                units[1] - manualUnits[1],
                units[2] - manualUnits[2],
                units[3] - manualUnits[3],
                units[4] - manualUnits[4],
                units[5] - manualUnits[5],
                units[6] - manualUnits[6]
            };
            
            if (this == m_parent->getXEdit()) {
                if (unitVar.size() > 0) m_parent->setXUnitMultiplier(1.0 / unitVar.first());
                else m_parent->setXUnitMultiplier(1.0);
            }
            if (this == m_parent->getYEdit()) {
                if (unitVar.size() > 0) m_parent->setYUnitMultiplier(1.0 / unitVar.first());
                else m_parent->setYUnitMultiplier(1.0);
            }
            
            QList<QString> unitStrs = {"m", "kg", "s", "A", "K", "mol", "cd"};
            m_autoUnit = "";
            bool withMul = false;
            for (int32_t i = 0; i<7; i++) {
                qreal uExp = unresolvedUnits.at(i);
                if (uExp != 0.0) {
                    if (withMul) m_autoUnit.append(cdotChar);
                    m_autoUnit.append(unitStrs.at(i));
                    if (uExp != 1.0) m_autoUnit.append(QString("^") + tr("%1").arg(uExp));
                    withMul = true;
                }
            }
            if (!m_autoUnit.isEmpty()) m_autoUnit = QString(" (") + m_autoUnit + QString(") ");
            updateBoundingRect();
        }
    }
}

void DiagramTitle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    QBrush selectBrush(QColor(192, 192, 255));
    
    painter->setRenderHint(QPainter::Antialiasing, false);
    
    if (hasFocus()) {
        painter->setPen(QPen(Qt::blue, 1, Qt::DotLine));
        painter->setBrush(QColor(255, 255, 127)); // Qt::lightyellow does not exist
        painter->drawRect(m_boundingRect);
    } else {
        if(m_text.isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::gray);
            painter->drawRect(m_boundingRect);
        } else {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(m_boundingRect);
        }
    }
    
    // Selection highlighting: ------------------------------------------------------------
    if ( m_selectBegin >= 0 && m_selectEnd >= 0 && m_selectBegin != m_selectEnd) {
        qreal xBegin = getPositionForIndex(m_selectBegin);
        qreal xEnd = getPositionForIndex(m_selectEnd);
        QRectF normSelRect(xBegin, 0.0, xEnd-xBegin, m_boundingRect.height());
        painter->setPen(QPen(Qt::NoPen)); painter->setBrush(selectBrush);
        painter->drawRect(normSelRect);
    }
    
    qreal currentX = 0.0;
    painter->setPen(Qt::black);
    painter->setBrush(Qt::NoBrush);
    
    QString contStr = m_text;
    QString normStr = contStr.section('_', 0, 0);
    QString subStr = contStr.section('_', 1, -1);
    
    QColor textColor = Qt::black;
    painter->setPen(QPen(textColor, 1, Qt::SolidLine)); painter->setFont(m_font);
    painter->drawText(QPointF(currentX, 0), normStr);
    currentX += textWidth(normStr);
    
    painter->setPen(QPen(textColor, 1, Qt::SolidLine)); painter->setFont(m_subFont);
    qreal textVPos = m_fontMetrics.descent();
    painter->drawText(QPointF(currentX, textVPos), subStr);
    currentX += subtextWidth(subStr);
    
    painter->setPen(QPen(textColor, 1, Qt::SolidLine)); painter->setFont(m_font);
    painter->drawText(QPointF(currentX, 0), m_autoUnit);
    currentX += textWidth(m_autoUnit);
    
    if (m_cursorVisible) { // && i == m_cursorPos) {
        cursorPosUpdate();
        painter->setPen(QPen(Qt::black, 1));
        painter->drawLine(QPointF(m_cursorX+1, m_cursorY), QPointF(m_cursorX+1, m_cursorH));
        painter->setPen(QPen());
    }
}

void DiagramTitle::keyPressEvent(QKeyEvent* event)
{
    const bool altPressed = event->modifiers() & Qt::AltModifier;
    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
    Q_UNUSED(ctrlPressed);
    
    QString charToInsert = event->text();
    int eventKey = event->key();
    
    // Special characters:
    if (eventKey == Qt::Key_AsciiCircum || eventKey == Qt::Key_Dead_Circumflex) charToInsert = "^";
    
    if (charToInsert.length() == 1 && !isCommandKey(eventKey)) { // to prevent ' ' chars to be added when the shift key is pressed for capital letters
        if (charToInsert == "*" || charToInsert == " " || charToInsert == "^" || charToInsert == "/") {
            if (m_cursorPos > 0) { // Cannot have a mathematical expression start with an operator
                if (charToInsert == "*" || charToInsert == " ") {
                    
                    emit getParent()->itemDataChanged();
                } else if (charToInsert == "^") {
                    
                    emit getParent()->itemDataChanged();
                } else if (charToInsert == "/") {
                    
                    emit getParent()->itemDataChanged();
                }
            }
        } else if (allowedCharacters.contains(charToInsert)) {
            insertText(charToInsert);
            emit getParent()->itemDataChanged();
        }
    }
    // The inserting has been done, now determine if any action is needed
    if (eventKey == Qt::Key_Backspace) {
        backspace();
        emit getParent()->itemDataChanged();
    } else if (eventKey == Qt::Key_Delete) {
        del();
        emit getParent()->itemDataChanged();
    } else if (eventKey == Qt::Key_Left) {
        if (shiftPressed) shiftLeftArrow();
        else leftArrow();
    } else if (eventKey == Qt::Key_Right) {
        if (shiftPressed) shiftRightArrow();
        else rightArrow();
    } else if (eventKey == Qt::Key_Up) {
        upArrow();
    } else if (eventKey == Qt::Key_Down) {
        downArrow();
    } else if (eventKey == Qt::Key_Enter || eventKey == Qt::Key_Return) {
        
    } else {
        if (!altPressed) {
            QGraphicsObject::keyPressEvent(event);
        }
    }
    
    
    cursorPosUpdate();
    updateBoundingRect();
    update(boundingRect()); // triggers paint()
    event->accept();
}

void DiagramTitle::insertText(const QString& inText) {
    insertTextAt(m_cursorPos, inText);
}

void DiagramTitle::insertTextAt(int64_t pos, const QString& inText) {
    QString text = inText;
    if (pos > m_text.length()) pos = m_text.length();
    if(pos>0){
        if (m_text.at(pos-1) == "\\") {
            m_text.removeAt(pos-1);
            m_cursorPos--; pos--;
            text.replace(m_cursorPos, 1, getGreekCharacter(inText));
        }
    }
    m_text.insert(pos, text);
    m_cursorPos += text.length();
}


void DiagramTitle::focusInEvent(QFocusEvent* event) {
    m_cursorTimer.start(500);
    m_cursorVisible = true;
    
    emit gainedFocus();
    QGraphicsObject::focusInEvent(event);
}

void DiagramTitle::focusOutEvent(QFocusEvent* event) {
    m_cursorTimer.stop();
    m_cursorVisible = false;
    m_selectBegin = m_selectEnd = m_selectAnchor = -1;
    
    emit lostFocus();
    QGraphicsObject::focusOutEvent(event);
}

void DiagramTitle::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void DiagramTitle::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void DiagramTitle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}
bool DiagramTitle::processAllContent() {
    return processContent(0, m_text.length());
}

bool DiagramTitle::processContent(int64_t selBegin, int64_t selEnd) {
    Q_UNUSED(selBegin); Q_UNUSED(selEnd);
    return true;
}


void DiagramTitle::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if(hasFocus()) {
        event->accept();
        grabMouse(); // "this" now gets all move events until release
        m_cursorPos = getCursorIndexForPosition(event->pos().x());
        m_selectBegin = m_selectEnd = m_selectAnchor = m_cursorPos;
        update(m_boundingRect);
    } else {
        QGraphicsObject::mousePressEvent(event);
    }
}

void DiagramTitle::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        int64_t mousePos = getCursorIndexForPosition(event->pos().x());
        m_selectBegin = std::min(m_selectAnchor, mousePos);
        m_selectEnd   = std::max(m_selectAnchor, mousePos);
        event->accept();
        update(m_boundingRect);
    }
    QGraphicsObject::mouseMoveEvent(event);
}

void DiagramTitle::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (scene() && scene()->mouseGrabberItem() == this) {
        ungrabMouse();
    }
    if (m_selectBegin == m_selectEnd) m_selectBegin = m_selectEnd = m_selectAnchor = -1; // If nothing was marked unset the selection
    QGraphicsObject::mouseReleaseEvent(event);
}

void DiagramTitle::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
}


int64_t DiagramTitle::getCursorIndexForPosition(qreal x) {
    
    QFontMetricsF fm(m_font);
    QFontMetricsF subFm(m_subFont);
    
    for (int64_t i = 0; i < m_text.size(); ++i) {
        //        if (m_content.at(i) == '#') { i++; if (i==m_content.size()) break; }
        QString strBeforeCursor(m_text); strBeforeCursor.truncate(i); // take only the part before position i
        
        qreal itemW1 = 0;
        qreal itemW2 = 0;
        qreal w = 0.0;
        bool inSubStr = false;
        QString normStr = strBeforeCursor.section('_', 0, 0);
        QString subStr = strBeforeCursor.section('_', 1, -1);
        if(m_cursorPos>=strBeforeCursor.indexOf('_') && strBeforeCursor.indexOf('_')>0) inSubStr = true;
        w = textWidth(normStr) + subtextWidth(subStr);
        if (i>0) {
            if (inSubStr) itemW1 = subFm.horizontalAdvance(m_text.at(i-1));
            else itemW1 = fm.horizontalAdvance(m_text.at(i-1));
        }
        if (inSubStr) itemW2 = subFm.horizontalAdvance(m_text.at(i));
        else itemW2 = fm.horizontalAdvance(m_text.at(i));
        
        // If the click is within this item's horizontal bounds, set cursor here
        if (m_text.at(i) == '_') {
            if (x >= w-0.6*itemW1 && x <= w) return i;
            if (x >= w && x <= w + 0.6*itemW2) return i+1;
        }
        else {
            if (x >= w-0.6*itemW1 && x <= w + 0.6*itemW2) return i;
        }
    }
    
    // If click is past all content, place cursor at the end
    return m_text.size();
}

qreal DiagramTitle::getPositionForIndex(int64_t id){
    QString strBeforeID(m_text); strBeforeID.truncate(id);
    
    qreal pos = 0.0;
    QString normStr = strBeforeID.section('_', 0, 0);
    QString subStr = strBeforeID.section('_', 1, -1);
    pos = textWidth(normStr) + subtextWidth(subStr);
    
    return pos;
}

void DiagramTitle::backspace() {
    if(m_cursorPos>0) {
        m_cursorPos--;
        m_text.remove(m_cursorPos,1);
    }
}
void DiagramTitle::del() {
    if(m_cursorPos<m_text.length()) {
        m_text.remove(m_cursorPos,1);
    }
}
void DiagramTitle::leftArrow() {
    m_selectBegin = m_selectEnd = m_selectAnchor = -1;
    if(m_cursorPos>0) {
        m_cursorPos -= 1;
    }
    cursorPosUpdate();
    update(boundingRect());
}
void DiagramTitle::shiftLeftArrow() {
    processAllContent();
    if(m_selectBegin == -1 && m_selectEnd == -1 && m_selectAnchor == -1) 
        m_selectAnchor = m_cursorPos;
    if(m_cursorPos>0) m_cursorPos -= 1;
    m_selectBegin = std::min(m_selectAnchor, m_cursorPos);
    m_selectEnd   = std::max(m_selectAnchor, m_cursorPos);
    
}
void DiagramTitle::rightArrow() {
    m_selectBegin = m_selectEnd = m_selectAnchor = -1;
    if(m_cursorPos<m_text.length()) {
        m_cursorPos += 1;
    }
    cursorPosUpdate();
    update(boundingRect());
}
void DiagramTitle::shiftRightArrow() {
    processAllContent();
    if(m_selectBegin == -1 && m_selectEnd == -1 && m_selectAnchor == -1) 
        m_selectAnchor = m_cursorPos;
    if(m_cursorPos<m_text.length()) m_cursorPos += 1;
    m_selectBegin = std::min(m_selectAnchor, m_cursorPos);
    m_selectEnd   = std::max(m_selectAnchor, m_cursorPos);
    
}
void DiagramTitle::upArrow() {
    
}
void DiagramTitle::downArrow() {
    
}
void DiagramTitle::cursorPosUpdate() {
    QString strBeforeCursor(m_text); strBeforeCursor.truncate(m_cursorPos);
    
    qreal w = 0.0;
    bool inSubStr = false;
    QString normStr = strBeforeCursor.section('_', 0, 0);
    QString subStr = strBeforeCursor.section('_', 1, -1);
    if(m_cursorPos>=strBeforeCursor.indexOf('_') && strBeforeCursor.indexOf('_')>0) inSubStr = true;
    w = textWidth(normStr) + subtextWidth(subStr);
    m_cursorX = w-1;
    m_cursorW = 2;
    if (!inSubStr) {
        m_cursorY = boundingRect().top();
        m_cursorH = boundingRect().bottom();
    }
    else {
        m_cursorY = boundingRect().top() +m_subFontMetrics.ascent() - m_fontMetrics.descent();
        m_cursorH = boundingRect().bottom() + m_subFontMetrics.descent();
    }
}

bool DiagramTitle::isCommandKey(const int key) {
    bool ret = false;
    if (key == Qt::Key_Backspace) { ret = true; }
    else if (key == Qt::Key_Delete) { ret = true; }
    else if (key == Qt::Key_Left) { ret = true; }
    else if (key == Qt::Key_Right) { ret = true; }
    else if (key == Qt::Key_Up) { ret = true; }
    else if (key == Qt::Key_Down) { ret = true; }
    else if (key == Qt::Key_Enter) { ret = true; }
    else if (key == Qt::Key_Return) { ret = true; }
    return ret;
}

qreal DiagramTitle::textWidth(const QString &text) {
    return m_fontMetrics.horizontalAdvance(text);
}
qreal DiagramTitle::subtextWidth(const QString &text) {
    return m_subFontMetrics.horizontalAdvance(text);
}

void DiagramTitle::toggleCursor() {
    m_cursorVisible = !m_cursorVisible;
    QRectF cursorRect = getCursorRect(); // in item coordinates
    update(cursorRect);
}
