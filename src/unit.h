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

#include <array>
#include <QString>
#include <QMap>

class Unit
{
public:
    // Seven SI base units: m, kg, s, A, K, mol, cd
    enum SIBaseUnit { METER, KILOGRAM, SECOND, AMPERE, KELVIN, MOLE, CANDELA, COUNT };
    
private:
    std::array<double, COUNT> exponents{};
    bool error_flag = false; // set true if invalid addition/subtraction
    
public:
    Unit();
    explicit Unit(const std::array<double, COUNT>& exps);
    
    // Accessors
    double getExp(SIBaseUnit u) const;
    void setExp(SIBaseUnit u, double val);
    
    // Error flag
    bool hasError() const;
    void clearError();
    
    // Arithmetic operations
    Unit operator*(const Unit& rhs) const;
    Unit operator/(const Unit& rhs) const;
    Unit pow(double n) const;
    Unit root(double n) const;
    Unit operator+(const Unit& rhs) const;
    Unit operator-(const Unit& rhs) const;
    
    bool operator==(const Unit& rhs) const;
    bool operator!=(const Unit& rhs) const;
    
    // Conversion: parse string like "kW·h"
    double convert(const QString& target) const;
    
    // Debug
    void print() const;
    
    // Some predefined SI base units
    static Unit meter();
    static Unit kilogram();
    static Unit second();
    static Unit ampere();
    static Unit kelvin();
    static Unit mole();
    static Unit candela();
};
