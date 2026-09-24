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

// data.h
#ifndef DATA_H
#define DATA_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QDebug> // For qDebug, qWarning
#include <limits> // Required for std::numeric_limits
#include "mathvariable.h"
/**
 * @brief The Data class manages a collection of named numeric variables.
 *
 * This class provides an interface to store and retrieve real-valued
 * numbers associated with QString variable names. It can be used,
 * for example, to store variables in a mathematical expression or
 * scientific application.
 */
class Data : public QObject
{
    Q_OBJECT
    
public:
    static constexpr double pi = 3.14159265358979323846;
    
    /**
     * @brief Constructs a Data object.
     * @param parent The parent QObject, if any.
     */
    explicit Data(QObject *parent = nullptr);
    
    MathVariable getValue(const QString &variableName) const;
    void setValue(const QString &variableName, MathVariable value);
    bool contains(const QString &variableName) const;
    void clear();
    void initialize();
    void deepCopy(Data *d);
    
    /**
     * @brief Removes a variable from the data collection.
     * @param variableName The name of the variable to remove.
     * @return True if the variable was removed, false if it did not exist.
     */
    bool removeValue(const QString &variableName);
    
    /**
     * @brief Gets a copy of the entire QMap of variables.
     * @return A QMap containing all variable names and their values.
     */
    QMap<QString, MathVariable> getAllVariables() const;
    
    /**
     * @brief Sets the entire QMap of variables.
     * This will replace the current map with the provided one.
     * @param variables The new QMap of variables to set.
     */
    void setAllVariables(const QMap<QString, MathVariable> &variables);
    
signals:
    /**
     * @brief variableNotFound This signal is emitted when a request is made
     * for a variable name that does not exist in the map.
     * @param variableName The name of the variable that was not found.
     */
    void variableNotFound(const QString &variableName) const;
    
private:
    QMap<QString, MathVariable> m_variables; /**< The QMap storing variable names and their numeric values. */
};

#endif // DATA_H
