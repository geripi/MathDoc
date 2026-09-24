/* Copyright (C) 2025, 2026 Gerald Pichler (gerald.pichler@chello.at)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// data.cpp
#include "data.h"
#include <limits> // Required for std::numeric_limits

/**
 * @brief Constructs a Data object.
 * @param parent The parent QObject, if any.
 */
Data::Data(QObject *parent)
: QObject(parent)
{
    // Constructor initializes the QMap (it's empty by default)
    initialize();
}

/**
 * @brief Gets the value associated with a given variable name.
 * @param variableName The name of the variable.
 * @return The numeric value of the variable. Returns std::numeric_limits<qreal>::max()
 * if the variable does not exist in the map. A warning is logged and a signal is emitted
 * if the variable is not found.
 */
MathVariable Data::getValue(const QString &variableName) const
{
    if (m_variables.contains(variableName)) {
        return m_variables.value(variableName);
    } else {
        qWarning() << "Data::getValue: Variable '" << variableName << "' not found. Returning max qreal value.";
        emit variableNotFound(variableName);
        return MathVariable(QList( {std::numeric_limits<qreal>::max()} )); // Return highest possible qreal value
    }
}

/**
 * @brief Sets the value for a given variable name.
 * If the variable already exists, its value is updated.
 * If it does not exist, a new entry is created.
 * @param variableName The name of the variable.
 * @param value The numeric value to set.
 */
void Data::setValue(const QString &variableName, MathVariable value)
{
    if(!variableName.isEmpty())
        m_variables.insert(variableName, value);
}

/**
 * @brief Checks if a variable with the given name exists.
 * @param variableName The name of the variable to check.
 * @return True if the variable exists, false otherwise.
 */
bool Data::contains(const QString &variableName) const
{
    return m_variables.contains(variableName);
}

void Data::clear()
{
    m_variables.clear();
    initialize();
}
void Data::initialize()
{
    // Base units                            { "m","kg", "s", "A", "K","mol", "cd" }
    m_variables.insert(QStringLiteral( "rad"),
                       MathVariable({ 1.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }));
    m_variables.insert(QStringLiteral( "m"  ),
                       MathVariable({ 1.0 }, { 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }));
    m_variables.insert(QStringLiteral( "g"  ),
                       MathVariable({0.001}, { 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 }));
    m_variables.insert(QStringLiteral( "s"  ),
                       MathVariable({ 1.0 }, { 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0 }));
    m_variables.insert(QStringLiteral( "A"  ),
                       MathVariable({ 1.0 }, { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0 }));
    m_variables.insert(QStringLiteral( "K"  ),
                       MathVariable({ 1.0 }, { 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0 }));
    m_variables.insert(QStringLiteral( "mol"),
                       MathVariable({ 1.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 }));
    m_variables.insert(QStringLiteral( "cd" ),
                       MathVariable({ 1.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 }));
    // Derived SI units                    { "m","kg", "s", "A", "K","mol", "cd" }
    m_variables.insert(QStringLiteral("Hz"),
                       MathVariable({1.0}, { 0.0, 0.0,-1.0, 0.0, 0.0, 0.0, 0.0}));          // Hertz = 1/s
    m_variables.insert(QStringLiteral("N"),
                       MathVariable({1.0}, { 1.0, 1.0,-2.0, 0.0, 0.0, 0.0, 0.0}));        // Newton = kg·m/s²
    m_variables.insert(QStringLiteral("Pa"),
                       MathVariable({1.0}, {-1.0, 1.0,-2.0, 0.0, 0.0, 0.0, 0.0}));       // Pascal = N/m²
    m_variables.insert(QStringLiteral("J"),
                       MathVariable({1.0}, { 2.0, 1.0,-2.0, 0.0, 0.0, 0.0, 0.0}));        // Joule = N·m
    m_variables.insert(QStringLiteral("W"),
                       MathVariable({1.0}, { 2.0, 1.0,-3.0, 0.0, 0.0, 0.0, 0.0}));        // Watt = J/s
    m_variables.insert(QStringLiteral("C"),
                       MathVariable({1.0}, { 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0}));         // Coulomb = A·s
    m_variables.insert(QStringLiteral("V"),
                       MathVariable({1.0}, { 2.0, 1.0,-3.0,-1.0, 0.0, 0.0, 0.0}));       // Volt = W/A
    m_variables.insert(QStringLiteral("F"),
                       MathVariable({1.0}, {-2.0,-1.0, 4.0, 2.0, 0.0, 0.0, 0.0}));       // Farad = C/V
    m_variables.insert(QStringLiteral("Ω"),
                       MathVariable({1.0}, { 2.0, 1.0,-3.0,-2.0, 0.0, 0.0, 0.0}));       // Ohm = V/A
    m_variables.insert(QStringLiteral("Ohm"),
                       MathVariable({1.0}, { 2.0, 1.0,-3.0,-2.0, 0.0, 0.0, 0.0}));       // Ohm = V/A
    m_variables.insert(QStringLiteral("S"),
                       MathVariable({1.0}, {-2.0,-1.0, 3.0, 2.0, 0.0, 0.0, 0.0}));       // Siemens = 1/Ω
    m_variables.insert(QStringLiteral("Wb"),
                       MathVariable({1.0}, { 2.0, 1.0,-2.0,-1.0, 0.0, 0.0, 0.0}));       // Weber = V·s
    m_variables.insert(QStringLiteral("T"),
                       MathVariable({1.0}, { 0.0, 1.0,-2.0,-1.0, 0.0, 0.0, 0.0}));       // Tesla = Wb/m²
    m_variables.insert(QStringLiteral("H"),
                       MathVariable({1.0}, { 2.0, 1.0,-2.0,-2.0, 0.0, 0.0, 0.0}));       // Henry = Wb/A
    m_variables.insert(QStringLiteral("lm"),
                       MathVariable({1.0}, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0}));         // Lumen = cd·sr
    m_variables.insert(QStringLiteral("lx"),
                       MathVariable({1.0}, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0}));         // Lux = lm/m²
    m_variables.insert(QStringLiteral("Bq"),
                       MathVariable({1.0}, { 0.0, 0.0,-1.0, 0.0, 0.0, 0.0, 0.0}));        // Becquerel = 1/s
    m_variables.insert(QStringLiteral("Gy"),
                       MathVariable({1.0}, { 2.0, 0.0,-2.0, 0.0, 0.0, 0.0, 0.0}));        // Gray = J/kg
    m_variables.insert(QStringLiteral("Sv"),
                       MathVariable({1.0}, { 2.0, 0.0,-2.0, 0.0, 0.0, 0.0, 0.0}));        // Sievert = J/kg
    m_variables.insert(QStringLiteral("kat"),
                       MathVariable({1.0}, { 0.0, 0.0,-1.0, 0.0, 0.0, 1.0, 0.0}));        // Katal = mol/s
    m_variables.insert(QStringLiteral("min"),
                       MathVariable({  60.0}, { 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0}));        // Minutes
    m_variables.insert(QStringLiteral("h"),
                       MathVariable({3600.0}, { 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0}));        // Hours
    m_variables.insert(QStringLiteral("U"),
                       MathVariable({2*pi}, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}));        // Umdrehungen
    m_variables.insert(QStringLiteral("rpm"),
                       MathVariable({pi/30.0}, { 0.0, 0.0,-1.0, 0.0, 0.0, 0.0, 0.0}));        // Rounds per minute
    m_variables.insert(QStringLiteral("deg"),
                       MathVariable({pi/180.0}, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}));   // Degrees
    m_variables.insert(QStringLiteral("°"),
                       MathVariable({pi/180.0}, { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}));   // Degrees
    
    // --- SI prefixes ---
    const QMap<QString, double> prefixes = {
        {"Y", 1e24}, {"Z", 1e21}, {"E", 1e18}, {"P", 1e15},
        {"T", 1e12}, {"G", 1e9},  {"M", 1e6},  {"k", 1e3},
        {"h", 1e2},  {"da", 1e1}, {"d", 1e-1}, {"c", 1e-2},
        {"m", 1e-3}, {"µ", 1e-6}, {"u", 1e-6},  // allow both µ and u
        {"n", 1e-9}, {"p", 1e-12}, {"f", 1e-15},
        {"a", 1e-18}, {"z", 1e-21}, {"y", 1e-24}
    };
    
    // --- Generate prefixed units ---
    const QList<QString> baseUnits = m_variables.keys();
    
    for (auto pIt = prefixes.constBegin(); pIt != prefixes.constEnd(); ++pIt) {
        const QString prefix = pIt.key();
        const qreal pfactor = pIt.value();
        
        for (const QString &baseUnit : baseUnits) {
            QString symbol = prefix + baseUnit;
            qreal val = m_variables[baseUnit].first();
            m_variables.insert(symbol, MathVariable({val*pfactor}, m_variables[baseUnit].unit()));
        }
    }
    
    // constants
    m_variables.insert(QStringLiteral("π"), MathVariable({ pi })); // π
    m_variables.insert(QStringLiteral("e"), MathVariable({ 2.71828182845904523536 })); // e
}

void Data::deepCopy(Data *d) {
    m_variables.clear();
    m_variables = d->getAllVariables();
}

/**
 * @brief Removes a variable from the data collection.
 * @param variableName The name of the variable to remove.
 * @return True if the variable was removed, false if it did not exist.
 */
bool Data::removeValue(const QString &variableName)
{
    // remove() returns the number of items removed (0 or 1 for QMap)
    return m_variables.remove(variableName) > 0;
}

/**
 * @brief Gets a copy of the entire QMap of variables.
 * @return A QMap containing all variable names and their values.
 */
QMap<QString, MathVariable> Data::getAllVariables() const
{
    return m_variables; // QMap is implicitly shared, so returning by value is efficient
}

/**
 * @brief Sets the entire QMap of variables.
 * This will replace the current map with the provided one.
 * @param variables The new QMap of variables to set.
 */
void Data::setAllVariables(const QMap<QString, MathVariable> &variables)
{
    m_variables = variables; // QMap uses implicit sharing, so this is efficient
}
