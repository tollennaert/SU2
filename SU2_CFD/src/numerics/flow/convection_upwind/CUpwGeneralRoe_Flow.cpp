/*!
 * \file CUpwGeneralRoe_Flow.cpp
 * \brief Implementation of numerics class CUpwGeneralRoe_Flow.
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

#include "../../../../include/numerics/flow/convection_upwind/CUpwGeneralRoe_Flow.hpp"

CUpwGeneralRoe_Flow::CUpwGeneralRoe_Flow(unsigned short val_nDim, unsigned short val_nVar, CConfig *config) : CNumerics(val_nDim, val_nVar, config) {

  implicit = (config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT);
  /* A grid is defined as dynamic if there's rigid grid movement or grid deformation AND the problem is time domain */
  dynamic_grid = config->GetDynamic_Grid();
  kappa = config->GetRoe_Kappa(); // 1 is unstable


  Diff_U = new su2double [nVar];
  Velocity_i = new su2double [nDim];
  Velocity_j = new su2double [nDim];
  RoeVelocity = new su2double [nDim];
  delta_vel  = new su2double [nDim];
  delta_wave = new su2double [nVar];
  ProjFlux_i = new su2double [nVar];
  ProjFlux_j = new su2double [nVar];
  Lambda = new su2double [nVar];
  Epsilon = new su2double [nVar];
  P_Tensor = new su2double* [nVar];
  invP_Tensor = new su2double* [nVar];

  for (iVar = 0; iVar < nVar; iVar++) {
    P_Tensor[iVar] = new su2double [nVar];
    invP_Tensor[iVar] = new su2double [nVar];
  }
}

CUpwGeneralRoe_Flow::~CUpwGeneralRoe_Flow(void) {

  delete [] Diff_U;
  delete [] Velocity_i;
  delete [] Velocity_j;
  delete [] RoeVelocity;
  delete [] delta_vel;
  delete [] delta_wave;
  delete [] ProjFlux_i;
  delete [] ProjFlux_j;
  delete [] Lambda;
  delete [] Epsilon;
  for (iVar = 0; iVar < nVar; iVar++) {
    delete [] P_Tensor[iVar];
    delete [] invP_Tensor[iVar];
  }
  delete [] P_Tensor;
  delete [] invP_Tensor;

}

void CUpwGeneralRoe_Flow::ComputeResidual(su2double *val_residual, su2double **val_Jacobian_i, su2double **val_Jacobian_j, CConfig *config) {

  AD::StartPreacc();
  AD::SetPreaccIn(V_i, nDim+4); AD::SetPreaccIn(V_j, nDim+4); AD::SetPreaccIn(Normal, nDim);
  AD::SetPreaccIn(S_i, 2); AD::SetPreaccIn(S_j, 2);
  if (dynamic_grid) {
    AD::SetPreaccIn(GridVel_i, nDim); AD::SetPreaccIn(GridVel_j, nDim);
  }
  su2double U_i[5] = {0.0,0.0,0.0,0.0,0.0}, U_j[5] = {0.0,0.0,0.0,0.0,0.0};

  /*--- Face area (norm or the normal vector) ---*/

  Area = 0.0;
  for (iDim = 0; iDim < nDim; iDim++)
  Area += Normal[iDim]*Normal[iDim];
  Area = sqrt(Area);

  /*-- Unit Normal ---*/

  for (iDim = 0; iDim < nDim; iDim++)
    UnitNormal[iDim] = Normal[iDim]/Area;

  /*--- Primitive variables at point i ---*/

  Velocity2_i = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) {
    Velocity_i[iDim] = V_i[iDim+1];
    Velocity2_i += Velocity_i[iDim]*Velocity_i[iDim];
  }

  Pressure_i = V_i[nDim+1];
  Density_i = V_i[nDim+2];
  Enthalpy_i = V_i[nDim+3];
  Energy_i = Enthalpy_i - Pressure_i/Density_i;
  StaticEnthalpy_i = Enthalpy_i - 0.5*Velocity2_i;
  StaticEnergy_i = StaticEnthalpy_i - Pressure_i/Density_i;

  Kappa_i = S_i[1]/Density_i;
  Chi_i = S_i[0] - Kappa_i*StaticEnergy_i;
  SoundSpeed_i = sqrt(Chi_i + StaticEnthalpy_i*Kappa_i);

  /*--- Primitive variables at point j ---*/


  Velocity2_j = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) {
    Velocity_j[iDim] = V_j[iDim+1];
    Velocity2_j += Velocity_j[iDim]*Velocity_j[iDim];
  }

  Pressure_j = V_j[nDim+1];
  Density_j = V_j[nDim+2];
  Enthalpy_j = V_j[nDim+3];
  Energy_j = Enthalpy_j - Pressure_j/Density_j;

  StaticEnthalpy_j = Enthalpy_j - 0.5*Velocity2_j;
  StaticEnergy_j = StaticEnthalpy_j - Pressure_j/Density_j;

  Kappa_j = S_j[1]/Density_j;
  Chi_j = S_j[0] - Kappa_j*StaticEnergy_j;
  SoundSpeed_j = sqrt(Chi_j + StaticEnthalpy_j*Kappa_j);

  /*--- Recompute conservative variables ---*/

  U_i[0] = Density_i; U_j[0] = Density_j;
  for (iDim = 0; iDim < nDim; iDim++) {
    U_i[iDim+1] = Density_i*Velocity_i[iDim]; U_j[iDim+1] = Density_j*Velocity_j[iDim];
  }
  U_i[nDim+1] = Density_i*Energy_i; U_j[nDim+1] = Density_j*Energy_j;

//  /*--- Roe-averaged variables at interface between i & j ---*/

    ComputeRoeAverage();

    if (RoeSoundSpeed2 <= 0.0) {
    for (iVar = 0; iVar < nVar; iVar++) {
      val_residual[iVar] = 0.0;
      for (jVar = 0; jVar < nVar; jVar++) {
      val_Jacobian_i[iVar][iVar] = 0.0;
      val_Jacobian_j[iVar][iVar] = 0.0;
      }
    }
      return;
    }

    RoeSoundSpeed = sqrt(RoeSoundSpeed2);

  /*--- Compute ProjFlux_i ---*/
  GetInviscidProjFlux(&Density_i, Velocity_i, &Pressure_i, &Enthalpy_i, Normal, ProjFlux_i);

  /*--- Compute ProjFlux_j ---*/
  GetInviscidProjFlux(&Density_j, Velocity_j, &Pressure_j, &Enthalpy_j, Normal, ProjFlux_j);

  /*--- Compute P and Lambda (do it with the Normal) ---*/

  GetPMatrix(&RoeDensity, RoeVelocity, &RoeSoundSpeed, &RoeEnthalpy, &RoeChi, &RoeKappa, UnitNormal, P_Tensor);

  ProjVelocity = 0.0; ProjVelocity_i = 0.0; ProjVelocity_j = 0.0;
  for (iDim = 0; iDim < nDim; iDim++) {
    ProjVelocity   += RoeVelocity[iDim]*UnitNormal[iDim];
    ProjVelocity_i += Velocity_i[iDim]*UnitNormal[iDim];
    ProjVelocity_j += Velocity_j[iDim]*UnitNormal[iDim];
  }

  /*--- Projected velocity adjustment due to mesh motion ---*/
  if (dynamic_grid) {
    su2double ProjGridVel = 0.0;
    for (iDim = 0; iDim < nDim; iDim++) {
      ProjGridVel   += 0.5*(GridVel_i[iDim]+GridVel_j[iDim])*UnitNormal[iDim];
    }
    ProjVelocity   -= ProjGridVel;
    ProjVelocity_i -= ProjGridVel;
    ProjVelocity_j -= ProjGridVel;
  }

  /*--- Flow eigenvalues and entropy correctors ---*/
  for (iDim = 0; iDim < nDim; iDim++)
    Lambda[iDim] = ProjVelocity;

  Lambda[nVar-2] = ProjVelocity + RoeSoundSpeed;
  Lambda[nVar-1] = ProjVelocity - RoeSoundSpeed;

  /*--- Compute absolute value with Mavriplis' entropy correction ---*/

  MaxLambda = fabs(ProjVelocity) + RoeSoundSpeed;
  Delta = config->GetEntropyFix_Coeff();

  for (iVar = 0; iVar < nVar; iVar++) {
    Lambda[iVar] = max(fabs(Lambda[iVar]), Delta*MaxLambda);
   }

//  /*--- Harten and Hyman (1983) entropy correction ---*/
//  for (iDim = 0; iDim < nDim; iDim++)
//    Epsilon[iDim] = 4.0*max(0.0, max(Lambda[iDim]-ProjVelocity_i, ProjVelocity_j-Lambda[iDim]));
//
//  Epsilon[nVar-2] = 4.0*max(0.0, max(Lambda[nVar-2]-(ProjVelocity_i+SoundSpeed_i),(ProjVelocity_j+SoundSpeed_j)-Lambda[nVar-2]));
//  Epsilon[nVar-1] = 4.0*max(0.0, max(Lambda[nVar-1]-(ProjVelocity_i-SoundSpeed_i),(ProjVelocity_j-SoundSpeed_j)-Lambda[nVar-1]));
//
//  for (iVar = 0; iVar < nVar; iVar++)
//    if ( fabs(Lambda[iVar]) < Epsilon[iVar] )
//      Lambda[iVar] = (Lambda[iVar]*Lambda[iVar] + Epsilon[iVar]*Epsilon[iVar])/(2.0*Epsilon[iVar]);
//    else
//      Lambda[iVar] = fabs(Lambda[iVar]);

//  for (iVar = 0; iVar < nVar; iVar++)
//    Lambda[iVar] = fabs(Lambda[iVar]);

  if (!implicit) {

    /*--- Compute wave amplitudes (characteristics) ---*/
    proj_delta_vel = 0.0;
    for (iDim = 0; iDim < nDim; iDim++) {
      delta_vel[iDim] = Velocity_j[iDim] - Velocity_i[iDim];
      proj_delta_vel += delta_vel[iDim]*Normal[iDim];
    }
    delta_p = Pressure_j - Pressure_i;
    delta_rho = Density_j - Density_i;
    proj_delta_vel = proj_delta_vel/Area;

    if (nDim == 2) {
      delta_wave[0] = delta_rho - delta_p/(RoeSoundSpeed*RoeSoundSpeed);
      delta_wave[1] = UnitNormal[1]*delta_vel[0]-UnitNormal[0]*delta_vel[1];
      delta_wave[2] = proj_delta_vel + delta_p/(RoeDensity*RoeSoundSpeed);
      delta_wave[3] = -proj_delta_vel + delta_p/(RoeDensity*RoeSoundSpeed);
    } else {
      delta_wave[0] = delta_rho - delta_p/(RoeSoundSpeed*RoeSoundSpeed);
      delta_wave[1] = UnitNormal[0]*delta_vel[2]-UnitNormal[2]*delta_vel[0];
      delta_wave[2] = UnitNormal[1]*delta_vel[0]-UnitNormal[0]*delta_vel[1];
      delta_wave[3] = proj_delta_vel + delta_p/(RoeDensity*RoeSoundSpeed);
      delta_wave[4] = -proj_delta_vel + delta_p/(RoeDensity*RoeSoundSpeed);
    }

    /*--- Roe's Flux approximation ---*/
    for (iVar = 0; iVar < nVar; iVar++) {
      val_residual[iVar] = 0.5*(ProjFlux_i[iVar]+ProjFlux_j[iVar]);
      for (jVar = 0; jVar < nVar; jVar++)
        val_residual[iVar] -= 0.5*Lambda[jVar]*delta_wave[jVar]*P_Tensor[iVar][jVar]*Area;
    }

    /*--- Flux contribution due to grid motion ---*/
    if (dynamic_grid) {
      ProjVelocity = 0.0;
      for (iDim = 0; iDim < nDim; iDim++)
        ProjVelocity += 0.5*(GridVel_i[iDim]+GridVel_j[iDim])*Normal[iDim];
      for (iVar = 0; iVar < nVar; iVar++) {
        val_residual[iVar] -= ProjVelocity * 0.5*(U_i[iVar]+U_j[iVar]);
      }
    }
  }
  else {

    /*--- Compute inverse P ---*/

    GetPMatrix_inv(invP_Tensor, &RoeDensity, RoeVelocity, &RoeSoundSpeed, &RoeChi , &RoeKappa, UnitNormal);

     /*--- Jacobians of the inviscid flux, scaled by
      kappa because val_resconv ~ kappa*(fc_i+fc_j)*Normal ---*/

    GetInviscidProjJac(Velocity_i, &Enthalpy_i, &Chi_i, &Kappa_i, Normal, kappa, val_Jacobian_i);

    GetInviscidProjJac(Velocity_j, &Enthalpy_j, &Chi_j, &Kappa_j, Normal, kappa, val_Jacobian_j);


    /*--- Diference variables iPoint and jPoint ---*/
    for (iVar = 0; iVar < nVar; iVar++)
      Diff_U[iVar] = U_j[iVar]-U_i[iVar];

    /*--- Roe's Flux approximation ---*/
    for (iVar = 0; iVar < nVar; iVar++) {
      val_residual[iVar] = kappa*(ProjFlux_i[iVar]+ProjFlux_j[iVar]);
      for (jVar = 0; jVar < nVar; jVar++) {
        Proj_ModJac_Tensor_ij = 0.0;

        /*--- Compute |Proj_ModJac_Tensor| = P x |Lambda| x inverse P ---*/

        for (kVar = 0; kVar < nVar; kVar++)
          Proj_ModJac_Tensor_ij += P_Tensor[iVar][kVar]*Lambda[kVar]*invP_Tensor[kVar][jVar];

        val_residual[iVar] -= (1.0-kappa)*Proj_ModJac_Tensor_ij*Diff_U[jVar]*Area;
        val_Jacobian_i[iVar][jVar] += (1.0-kappa)*Proj_ModJac_Tensor_ij*Area;
        val_Jacobian_j[iVar][jVar] -= (1.0-kappa)*Proj_ModJac_Tensor_ij*Area;
      }
    }

    /*--- Jacobian contributions due to grid motion ---*/
    if (dynamic_grid) {
      ProjVelocity = 0.0;
      for (iDim = 0; iDim < nDim; iDim++)
        ProjVelocity += 0.5*(GridVel_i[iDim]+GridVel_j[iDim])*Normal[iDim];
      for (iVar = 0; iVar < nVar; iVar++) {
        val_residual[iVar] -= ProjVelocity * 0.5*(U_i[iVar]+U_j[iVar]);
        /*--- Implicit terms ---*/
        val_Jacobian_i[iVar][iVar] -= 0.5*ProjVelocity;
        val_Jacobian_j[iVar][iVar] -= 0.5*ProjVelocity;
      }
    }

  }

  AD::SetPreaccOut(val_residual, nVar);
  AD::EndPreacc();
}


void CUpwGeneralRoe_Flow::ComputeRoeAverage() {

  //su2double delta_rhoStaticEnergy, err_P, s, D;
  // su2double tol = 10-6;

  R = sqrt(fabs(Density_j/Density_i));
  RoeDensity = R*Density_i;
  sq_vel = 0;  for (iDim = 0; iDim < nDim; iDim++) {
    RoeVelocity[iDim] = (R*Velocity_j[iDim]+Velocity_i[iDim])/(R+1);
    sq_vel += RoeVelocity[iDim]*RoeVelocity[iDim];
  }

  RoeEnthalpy = (R*Enthalpy_j+Enthalpy_i)/(R+1);
  delta_rho = Density_j - Density_i;
  delta_p = Pressure_j - Pressure_i;
  RoeKappa = 0.5*(Kappa_i + Kappa_j);
  RoeKappa = (Kappa_i + Kappa_j + 4*RoeKappa)/6;
  RoeChi = 0.5*(Chi_i + Chi_j);
  RoeChi = (Chi_i + Chi_j + 4*RoeChi)/6;


//  RoeKappaStaticEnthalpy = 0.5*(StaticEnthalpy_i*Kappa_i + StaticEnthalpy_j*Kappa_j);
//  RoeKappaStaticEnthalpy = (StaticEnthalpy_i*Kappa_i + StaticEnthalpy_j*Kappa_j + 4*RoeKappaStaticEnthalpy)/6;
//  s = RoeChi + RoeKappaStaticEnthalpy;
//  D = s*s*delta_rho*delta_rho + delta_p*delta_p;
//  delta_rhoStaticEnergy = Density_j*StaticEnergy_j - Density_i*StaticEnergy_i;
//  err_P = delta_p - RoeChi*delta_rho - RoeKappa*delta_rhoStaticEnergy;
//
//
//  if (abs((D - delta_p*err_P)/Density_i)>1e-3 && abs(delta_rho/Density_i)>1e-3 && s/Density_i > 1e-3) {
//
//    RoeKappa = (D*RoeKappa)/(D - delta_p*err_P);
//    RoeChi = (D*RoeChi+ s*s*delta_rho*err_P)/(D - delta_p*err_P);
//
//  }

  RoeSoundSpeed2 = RoeChi + RoeKappa*(RoeEnthalpy-0.5*sq_vel);

}
