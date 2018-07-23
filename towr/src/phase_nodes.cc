/******************************************************************************
Copyright (c) 2018, Alexander W. Winkler. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include <towr/variables/phase_nodes.h>
#include <towr/variables/cartesian_dimensions.h>

#include <iostream>

namespace towr {

void
PhaseNodes::SetBoundsEEMotion ()
{
  for (int idx=0; idx<GetRows(); ++idx) {
    auto nvi = GetNodeValuesInfo(idx).front(); // bound idx by first node it represents

    // stance-node:
    // Phase-based End-effector Parameterization:
    // end-effector is not allowed to move if in stance phase
    if (IsConstantNode(nvi.id_)) {
      if (nvi.deriv_ == kVel)
        bounds_.at(idx) = ifopt::BoundZero;

    // swing node:
    // These are only the nodes where both the polynomial to the left and to the
    // right represent a swing-phase -> Swing nodes don't exist if choosing
    // only one polynomial for each swing-phase.
    } else {
      // zero velocity in z direction. Since we are typically choosing two
      // polynomials per swing-phase, this restricts the swing
      // to have reached it's extreme at half-time and creates smoother
      // stepping motions.
      //
      // In contrast to the above bounds, these are more hacky and less general,
      // and could be removed after e.g. adding a cost that penalizes
      // endeffector accelerations.
      if (nvi.deriv_ == kVel && nvi.dim_ == Z)
        bounds_.at(idx) = ifopt::BoundZero;
    }
  }
}

void
PhaseNodes::SetBoundsEEForce ()
{
  for (int idx=0; idx<GetRows(); ++idx) {
    NodeValueInfo nvi = GetNodeValuesInfo(idx).front(); // only one node anyway

    // swing node
    // Phase-based End-effector Parameterization
    if (IsConstantNode(nvi.id_)) {
      bounds_.at(idx) = ifopt::BoundZero; // force and derivative must be zero during swing-phase
    }
  }
}

std::vector<PhaseNodes::PolyInfo>
BuildPolyInfos (int phase_count, bool is_in_contact_at_start,
                int n_polys_in_changing_phase, PhaseNodes::Type type)
{
  using PolyInfo = PhaseNodes::PolyInfo;
  std::vector<PolyInfo> polynomial_info;

  bool first_phase_constant = (is_in_contact_at_start  && type==PhaseNodes::Motion)
                           || (!is_in_contact_at_start && type==PhaseNodes::Force);


  bool phase_constant = first_phase_constant;

  for (int i=0; i<phase_count; ++i) {
    if (phase_constant)
      polynomial_info.push_back(PolyInfo(i,0,1, true));
    else
      for (int j=0; j<n_polys_in_changing_phase; ++j)
        polynomial_info.push_back(PolyInfo(i,j,n_polys_in_changing_phase, false));

    phase_constant = !phase_constant; // constant and non-constant phase alternate
  }

  return polynomial_info;
}

PhaseNodes::PhaseNodes (int phase_count,
                        bool is_in_contact_at_start,
                        const std::string& name,
                        int n_polys_in_changing_phase,
                        Type type)
    :NodesVariables(name)
{
  polynomial_info_ = BuildPolyInfos(phase_count, is_in_contact_at_start, n_polys_in_changing_phase, type);

  int n_dim = k3D;
  SetIdxToNvis(polynomial_info_, n_dim);
  int n_nodes = polynomial_info_.size()+1;

  InitMembers(n_nodes, n_dim, idx_to_nvis_.size());

  if (type == Motion)
    SetBoundsEEMotion();
  else if (type == Force)
    SetBoundsEEForce();
  else
    assert(false); // phase-node type not defined
}

PhaseNodes::VecDurations
PhaseNodes::ConvertPhaseToPolyDurations(const VecDurations& phase_durations) const
{
  VecDurations poly_durations;

  for (int i=0; i<GetPolynomialCount(); ++i) {
    auto info = polynomial_info_.at(i);
    poly_durations.push_back(phase_durations.at(info.phase_)/info.n_polys_in_phase_);
  }

  return poly_durations;
}

double
PhaseNodes::GetDerivativeOfPolyDurationWrtPhaseDuration (int poly_id) const
{
  int n_polys_in_phase = polynomial_info_.at(poly_id).n_polys_in_phase_;
  return 1./n_polys_in_phase;
}

int
PhaseNodes::GetNumberOfPrevPolynomialsInPhase(int poly_id) const
{
  return polynomial_info_.at(poly_id).poly_in_phase_;
}

void
PhaseNodes::SetIdxToNvis (const std::vector<PolyInfo>& polynomial_info, int n_dim)
{
  idx_to_nvis_.clear();

  int idx_start = 0;


  // define variables for first node manually
  NodeValueInfo nvi;
  nvi.id_ = 0;
  for (int dim=0; dim<n_dim; ++dim) {
    nvi.dim_ = dim;

    nvi.deriv_ = kPos;
    idx_to_nvis_[idx_start+dim].push_back(nvi);
    nvi.deriv_ = kVel;
    idx_to_nvis_[idx_start+n_dim+dim].push_back(nvi);
  }



  // go through all polynomial and only look at end node
  for (int i=0; i<polynomial_info.size(); ++i) {

    // if constant polynomial, same index is used,
    // if non constant, end node gets separate index
    if (!polynomial_info.at(i).is_constant_) {
      idx_start += Node::n_derivatives*n_dim;
    }

    // node end
    nvi.id_ = GetNodeId(i, Side::End);
    for (int dim=0; dim<n_dim; ++dim) {
      nvi.dim_ = dim;

      nvi.deriv_ = kPos;
      idx_to_nvis_[idx_start+dim].push_back(nvi);
      nvi.deriv_ = kVel;
      idx_to_nvis_[idx_start+n_dim+dim].push_back(nvi);
    }
  }
}

std::vector<PhaseNodes::NodeValueInfo>
PhaseNodes::GetNodeValuesInfo(int idx) const
{
  return idx_to_nvis_.at(idx);
}

bool
PhaseNodes::IsConstantNode (int node_id) const
{
  bool is_constant = false;

  // node is considered constant if either left or right polynomial
  // belongs to a constant phase
  for (int poly_id : GetAdjacentPolyIds(node_id))
    if (IsInConstantPhase(poly_id))
      is_constant = true;

  return is_constant;
}

bool
PhaseNodes::IsInConstantPhase(int poly_id) const
{
  return polynomial_info_.at(poly_id).is_constant_;
}

PhaseNodes::NodeIds
PhaseNodes::GetIndicesOfNonConstantNodes() const
{
  NodeIds node_ids;

  for (int id=0; id<GetNodes().size(); ++id)
    if (!IsConstantNode(id))
      node_ids.push_back(id);

  return node_ids;
}

int
PhaseNodes::GetPhase (int node_id) const
{
  assert(!IsConstantNode(node_id)); // because otherwise it has two phases

  int poly_id = GetAdjacentPolyIds(node_id).front();
  return polynomial_info_.at(poly_id).phase_;
}

int
PhaseNodes::GetPolyIDAtStartOfPhase (int phase) const
{
  int poly_id=0;
  for (int i=0; i<polynomial_info_.size(); ++i)
    if (polynomial_info_.at(i).phase_ == phase)
      return i;
}

Eigen::Vector3d
PhaseNodes::GetValueAtStartOfPhase (int phase) const
{
  int node_id = GetNodeIDAtStartOfPhase(phase);
  return GetNodes().at(node_id).p();
}

int
PhaseNodes::GetNodeIDAtStartOfPhase (int phase) const
{
  int poly_id=GetPolyIDAtStartOfPhase(phase);
  return GetNodeId(poly_id, Side::Start);
}

std::vector<int>
PhaseNodes::GetAdjacentPolyIds (int node_id) const
{
  std::vector<int> poly_ids;
  int last_node_id = GetNodes().size()-1;

  if (node_id==0)
    poly_ids.push_back(0);
  else if (node_id==last_node_id)
    poly_ids.push_back(last_node_id-1);
  else {
    poly_ids.push_back(node_id-1);
    poly_ids.push_back(node_id);
  }

  return poly_ids;
}

PhaseNodes::PolyInfo::PolyInfo(int phase, int poly_id_in_phase,
                               int num_polys_in_phase, bool is_constant)
    :phase_(phase),
     poly_in_phase_(poly_id_in_phase),
     n_polys_in_phase_(num_polys_in_phase),
     is_constant_(is_constant)
{
}

} /* namespace towr */
