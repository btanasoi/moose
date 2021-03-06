/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/
#include "PackedColumn.h"

template<>
InputParameters validParams<PackedColumn>()
{
  InputParameters params = validParams<Material>();

  // Add a parameter to get the radius of the balls in the column (used later to interpolate permeability).
  params.addParam<Real>("ball_radius", "The radius of the steel balls that are packed in the column.  Used to interpolate _permeability.");

  return params;
}


PackedColumn::PackedColumn(const InputParameters & parameters) :
    Material(parameters),

    // Get the one parameter from the input file
    _ball_radius(getParam<Real>("ball_radius")),

    // Declare two material properties.  This returns references that we
    // hold onto as member variables
    _permeability(declareProperty<Real>("permeability")),
    _porosity(declareProperty<Real>("porosity")),
    _viscosity(declareProperty<Real>("viscosity")),
    _thermal_conductivity(declareProperty<Real>("thermal_conductivity")),
    _heat_capacity(declareProperty<Real>("heat_capacity")),
    _density(declareProperty<Real>("density"))
{
  std::vector<Real> ball_sizes = {1, 3};

  // From the paper: Table 1
  std::vector<Real> permeability = {0.8451e-9, 8.968e-9};

  // Set the x,y data on the LinearInterpolation object.
  _permeability_interpolation.setData(ball_sizes, permeability);
}

void
PackedColumn::computeQpProperties()
{
  _viscosity[_qp] = 7.98e-4; // (Pa*s) Water at 30 degrees C (Wikipedia)

  // Sample the LinearInterpolation object to get the permeability for the ball size
  _permeability[_qp] = 0.8451e-9; // _permeability_interpolation.sample(_ball_radius);


  // Compute the heat conduction material properties as a linear combination of
  // the material properties for water and steel.

  // We're assuming close packing  so the porosity will be 1 - 0.74048 = 0.25952
  // ( http://en.wikipedia.org/wiki/Close-packing_of_equal_spheres )
  _porosity[_qp] = 0.25952;

  // We will compute a "bulk" thermal conductivity, specific heat and density
  // as a linear combination of the water and steel (all from Wikipedia):
  Real water_k = 0.6;  // (W/m*K)
  Real water_cp = 4181.3; // (J/kg*K)
  Real water_rho = 995.6502;  // (kg/m^3 @ 303K)

  Real steel_k = 18;  // (W/m*K)
  Real steel_cp = 466;  // (J/kg*K)
  Real steel_rho = 8000;  // (kg/m^3)

  // Now actually set the value at the quadrature point
  _thermal_conductivity[_qp] = _porosity[_qp]*water_k + (1.0-_porosity[_qp])*steel_k;
  _density[_qp] = _porosity[_qp]*water_rho + (1.0-_porosity[_qp])*steel_rho;
  _heat_capacity[_qp] = _porosity[_qp]*water_cp*water_rho + (1.0-_porosity[_qp])*steel_cp*steel_rho;
}
