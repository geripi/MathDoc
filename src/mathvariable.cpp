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

#include "mathvariable.h"
#include <QStringList>
#include <QtGlobal>
#include <stdexcept>
#include <algorithm> // for std::max

// --- Constructors ---
MathVariable::MathVariable() {
    m_values = QList<qreal>({});
    m_unit = QList<qreal>({0,0,0,0,0,0,0}); // exponents of the base units in this order: { "m", "kg", "s", "A", "K", "mol", "cd" }
}

MathVariable::MathVariable(const QList<qreal>& values)
: m_values(values)
{
    m_unit = QList<qreal>({0,0,0,0,0,0,0}); // exponents of the base units in this order: { "m", "kg", "s", "A", "K", "mol", "cd" }
}
MathVariable::MathVariable(const QList<qreal>& values, const QList<qreal>& unit)
: m_values(values), m_unit(unit)
{
    if (m_unit.size() != 7) { throw std::invalid_argument("Unit vector size must be 7."); }
}

// --- Getters ---
const QList<qreal>& MathVariable::values() const { return m_values; }
const QList<qreal>& MathVariable::unit() const { return m_unit; }
qreal MathVariable::getUnitAt(int64_t i) const {
    if (m_unit.size() != 7) { throw std::invalid_argument("Unit vector size must be 7."); }
    if (i<0 || i>=7) { throw std::invalid_argument(QString("Unit index must be 0 to 6! Index given is %1!").arg(i).toStdString()); }
    return m_unit[i];
}

// --- Setters ---
void MathVariable::setValues(const QList<qreal>& values) { m_values = values; }
void MathVariable::setUnit(const QList<qreal>& unit) {
    if (unit.size() != 7) { throw std::invalid_argument("Unit vector size must be 7."); }
    m_unit = unit;
}
void MathVariable::setUnitAt(int64_t i, qreal unitExponent) {
    if (m_unit.size() != 7) { throw std::invalid_argument("Unit vector size must be 7."); }
    if (i<0 || i>=7) { throw std::invalid_argument(QString("Unit index must be 0 to 6! Index given is %1!").arg(i).toStdString()); }
    m_unit[i] = unitExponent;
}

// --- Utility ---
bool MathVariable::isValidValue() const {
    if (m_values.isEmpty()) return false;
    bool isValid = false;
    for (qreal f: m_values) {
        if (qIsFinite(f) &&
            f != std::numeric_limits<qreal>::max() &&
            f !=-std::numeric_limits<qreal>::max()) {
            isValid = true;
        }
    }
    return isValid;
}


bool MathVariable::hasSameUnit(const MathVariable& other) const {
    return m_unit == other.m_unit;
}

bool MathVariable::isDimensionless() const {
    bool b = true;
    for (qreal d: m_unit) {
        if (d != 0.0) {
            b = false;
            break;
        }
    }
    return b;
}


QString MathVariable::unitString() const {
    static const QStringList baseUnits = { "m", "kg", "s", "A", "K", "mol", "cd" };
    QStringList parts;
    for (int64_t i = 0; i < m_unit.size() && i < baseUnits.size(); ++i) {
        if (m_unit[i] != 0) {
            parts << QString("%1^%2").arg(baseUnits[i]).arg(m_unit[i]);
        }
    }
    return parts.isEmpty() ? "dimensionless" : parts.join(" * ");
}

// --- Comparison ---
bool MathVariable::operator==(const MathVariable& other) const {
    if (m_unit != other.m_unit) {
        return false; // units must match exactly
    }
    if (m_isUsedAsUnit != other.isUsedAsUnit()) {
        return false; // only compare units with units and general variables with general variables
    }
    if (m_values.size() != other.m_values.size()) {
        return false; // length mismatch
    }
    
    for (int64_t i = 0; i < m_values.size(); ++i) {
        if (!qFuzzyCompare(m_values[i], other.m_values[i])) {
            return false;
        }
    }
    return true;
}

bool MathVariable::operator!=(const MathVariable& other) const {
    return !(*this == other);
}

qreal& MathVariable::operator[](qsizetype index) {
//    qDebug() << "qreal& MathVariable::operator[]: m_values.size()=" << m_values.size() << ", index=" << index;
    if (index < 0 || index >= m_values.size()) {
        throw std::out_of_range("MathVariable index out of range");
    }
    return m_values[index];
}

const qreal& MathVariable::operator[](qsizetype index) const {
//    qDebug() << "qreal& MathVariable::operator[]: m_values.size()=" << m_values.size() << ", index=" << index;
    if (index < 0 || index >= m_values.size()) {
        throw std::out_of_range("MathVariable index out of range");
    }
    return m_values[index];
}

QDebug operator<<(QDebug dbg, const MathVariable& var)
{
    QDebugStateSaver saver(dbg); // preserve formatting
    dbg.nospace() << "Var(v=[";
    
    const QList<qreal>& vals = var.values();
    for (int64_t i = 0; i < vals.size(); ++i) {
        dbg.nospace() << vals[i];
        if (i < vals.size() - 1)
            dbg.nospace() << ", ";
    }
    dbg.nospace() << "], u=" << var.unitString() << ")";
    return dbg;
}
