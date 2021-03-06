/*
  Copyright (C) 2016,2017 The ESPResSo project

  This file is part of ESPResSo.


  ESPResSo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ESPResSo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "CylindricalLBFluxDensityProfileAtParticlePositions.hpp"
#include "lbgpu.hpp"
#include "utils.hpp"
#include "utils/Histogram.hpp"
#include "utils/coordinate_transformation.hpp"

namespace Observables {

std::vector<double> CylindricalLBFluxDensityProfileAtParticlePositions::
operator()(PartCfg &partCfg) const {
  std::vector<size_t> n_bins{{static_cast<size_t>(n_r_bins),
                              static_cast<size_t>(n_phi_bins),
                              static_cast<size_t>(n_z_bins)}};
  std::vector<std::pair<double, double>> limits{
      {std::make_pair(min_r, max_r), std::make_pair(min_phi, max_phi),
       std::make_pair(min_z, max_z)}};
  Utils::CylindricalHistogram<double> histogram(n_bins, 3, limits);
#ifdef LB_GPU
  // First collect all positions (since we want to call the LB function to
  // get the fluid velocities only once).
  std::vector<::Vector<3, double>> folded_positions;
  std::transform(ids().begin(), ids().end(),
                 std::back_inserter(folded_positions), [&partCfg](int id) {
                   return ::Vector<3, double>(folded_position(partCfg[id]));
                 });
  std::vector<double> ppos(3 * ids().size());
  for (auto it = folded_positions.begin(); it != folded_positions.end(); ++it) {
    size_t ind = std::distance(folded_positions.begin(), it);
    ppos[3 * ind + 0] = (*it)[0];
    ppos[3 * ind + 1] = (*it)[1];
    ppos[3 * ind + 2] = (*it)[2];
  }
  std::vector<double> velocities(3 * ids().size());
  lb_lbfluid_get_interpolated_velocity_at_positions(
      ppos.data(), velocities.data(), ids().size());
  for (auto &p : folded_positions)
    p -= center;
  for (int ind = 0; ind < ids().size(); ++ind) {
    histogram.update(Utils::transform_pos_to_cylinder_coordinates(
                         folded_positions[ind], axis),
                     Utils::transform_vel_to_cylinder_coordinates(
                         ::Vector<3, double>{{velocities[3 * ind + 0],
                                              velocities[3 * ind + 1],
                                              velocities[3 * ind + 2]}},
                         axis, folded_positions[ind]));
  }
  histogram.normalize();
#endif // LB_GPU
  return histogram.get_histogram();
}
} // namespace Observables
