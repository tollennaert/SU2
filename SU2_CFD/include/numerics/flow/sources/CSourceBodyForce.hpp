﻿/*!
 * \file CSourceBodyForce.hpp
 * \brief Delaration of numerics class CSourceBodyForce, the
 *        implementation is in the CSourceBodyForce.cpp file.
 * \author F. Palacios, T. Economon
 * \version 7.0.0 "Blackbird"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2019, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../../CNumerics.hpp"

/*!
 * \class CSourceBodyForce
 * \brief Class for the source term integration of a body force.
 * \ingroup SourceDiscr
 * \author T. Economon
 */
class CSourceBodyForce : public CNumerics {
  su2double *Body_Force_Vector;

public:

  /*!
   * \param[in] val_nDim - Number of dimensions of the problem.
   * \param[in] val_nVar - Number of variables of the problem.
   * \param[in] config - Definition of the particular problem.
   */
  CSourceBodyForce(unsigned short val_nDim, unsigned short val_nVar, CConfig *config);

  /*!
   * \brief Destructor of the class.
   */
  ~CSourceBodyForce(void);

  /*!
   * \brief Source term integration for a body force.
   * \param[out] val_residual - Pointer to the residual vector.
   * \param[in] config - Definition of the particular problem.
   */
  void ComputeResidual(su2double *val_residual, CConfig *config);

};
