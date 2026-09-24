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

#pragma once

#include <QList>
#include <QString>
#include <QDebug>


class MathVariable
{
private:
    QList<qreal> m_values;  // numerical values
    QList<qreal> m_unit;    // unit exponents (m, kg, s, A, K, mol, cd)
    
public:
    // --- Constructors ---
    MathVariable();
    MathVariable(const QList<qreal>& values);
    MathVariable(const QList<qreal>& values, const QList<qreal>& unit);
    MathVariable(const MathVariable&) = default;
    MathVariable& operator=(const MathVariable&) = default;
    
    // --- Getters ---
    const QList<qreal>& values() const;
    const QList<qreal>& unit() const;
    const qreal getUnitAt(int i) const;
    const qreal last() const { if (!m_values.isEmpty()) { return m_values.last(); } else { return std::numeric_limits<qreal>::max(); } }
    const qreal first() const { if (!m_values.isEmpty()) { return m_values.first(); } else { return std::numeric_limits<qreal>::max(); } }
    
    qreal& operator[](qsizetype index); // Access value by index (read/write)
    const qreal& operator[](qsizetype index) const; // Access value by index (read-only)
    const qreal& at(qsizetype index) const { return m_values.at(index); } // Access value by index (read-only)
    
    
    // Housekeeping
    qsizetype size() const { return m_values.size(); }
    void reserve(qsizetype n) { m_values.reserve(n); }
    void append(qreal value) { m_values.append(value); }
    
    // --- Setters ---
    void setValues(const QList<qreal>& values);
    void setUnit(const QList<qreal>& unit);
    void setUnitAt(int i, qreal unitExponent);
    void clear() { m_values.clear(); m_unit = QList<qreal>({0,0,0,0,0,0,0});}
    void setErrorValue(qreal e = std::numeric_limits<qreal>::max()) { clear(); m_values = QList<qreal>({e}); }
    
    // --- Utility ---
    bool isValidValue() const;
    bool hasSameUnit(const MathVariable& other) const;
    bool isDimensionless() const;
    bool isEmpty() const { return m_values.isEmpty(); }
    QString unitString() const;
    
    // --- Comparison ---
    bool operator==(const MathVariable& other) const;
    bool operator!=(const MathVariable& other) const;
    
    // Iterators for range-based for loops
    QList<qreal>::iterator begin() { return m_values.begin(); }
    QList<qreal>::iterator end()   { return m_values.end(); }
    
    QList<qreal>::const_iterator begin() const { return m_values.begin(); }
    QList<qreal>::const_iterator end()   const { return m_values.end(); }
    
    QList<qreal>::const_iterator cbegin() const { return m_values.cbegin(); }
    QList<qreal>::const_iterator cend()   const { return m_values.cend(); }
};

// Stream operator for qDebug()
QDebug operator<<(QDebug dbg, const MathVariable& var);

