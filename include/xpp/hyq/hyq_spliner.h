/**
@file    hyq_spliner.cpp
@author  Alexander Winkler (winklera@ethz.ch)
@date    Oct 21, 2014
@brief   Splines body position, orientation and swing leg
 */

#ifndef _XPP_HYQ_SPLINER_H_
#define _XPP_HYQ_SPLINER_H_

#include <xpp/hyq/hyq_state.h>
#include <xpp/utils/spliner_3d.h>
#include <xpp/zmp/zmp_spline.h>


namespace xpp {
namespace hyq {


struct SplineNode {
  typedef xpp::utils::Point3d Point3d;

  SplineNode(const HyqState& state, const Point3d& ori_rpy, double t_max)
      : state_(state), ori_rpy_(ori_rpy), T(t_max) {};

  HyqState state_;
  Point3d ori_rpy_; // fixme remove this and use quaternion orientation directly for interpolation
  double T;         // time to reach this state
};



/**
@brief Splines the base pose (position + orientation).

No velocity and accelerations of orientation, only roll, pitch, yaw.
For that transfer roll, pitch yaw velocities and accelerations into fixed global
frame values omega (rollPitchYawToEar)
 */
class HyqSpliner {
public:
  typedef xpp::zmp::ZmpSpline ZmpSpline;
  typedef std::vector<ZmpSpline> VecZmpSpline;
  typedef Eigen::Vector3d Vector3d;
  typedef Foothold::VecFoothold VecFoothold;
  typedef ::xpp::utils::QuinticSpliner Spliner;
  typedef ::xpp::utils::Spliner3d< Spliner > Spliner3d;
  typedef Spliner3d::Point Point;

public:
  HyqSpliner() {};
  virtual ~HyqSpliner() {};


  void SetParams(double upswing, double lift_height, double outward_swing_distance);
  void Init(const HyqState& P_init, const VecZmpSpline& zmp_splines,
            const VecFoothold& footholds, double robot_height);
  /**
  @brief function to access the current state AFTER states have been set.
  @param[in]  t_global global time.
  @return     curr position, orientation and current foothold of hyq.
   */
  HyqState GetSplinedState(double t_global) const;
  double GetTotalTime() const;
  SplineNode GetGoalNode(double t_global) const;

private:
  std::vector<SplineNode> nodes_; // the discrete states to spline through
  std::vector<Spliner3d> pos_spliner_, ori_spliner_;
  std::vector<LegDataMap< Spliner3d > > feet_spliner_up_, feet_spliner_down_;

  double kUpswingPercent;       // how long to swing up during swing
  double kLiftHeight;           // how high to lift the leg
  double kOutwardSwingDistance; // how far to swing leg outward (y-dir)

  /** Transform global time to local spline time dt */
  double GetLocalSplineTime(double t_global) const;



  static std::vector<SplineNode> BuildStateSequence(const HyqState& P_init,
                                            const VecZmpSpline& zmp_splines,
                                            const VecFoothold& footholds,
                                            double robot_height);

  void CreateAllSplines(const std::vector<SplineNode>& nodes);


  static Eigen::Vector3d TransformQuatToRpy(const Eigen::Quaterniond& q);

  /**
  @brief transforms a HyqState into a collection of Points, including
         body position, body orientation, and feet position
  @param[in] time_to_reach how long the robot has to achieve this state
   */
  static SplineNode BuildNode(const HyqState& state, double t_max);


  Spliner3d BuildPositionSpline(const SplineNode& from, const SplineNode& to) const;
  Spliner3d BuildOrientationRpySpline(const SplineNode& from, const SplineNode& to) const;
  LegDataMap<Spliner3d> BuildFootstepSplineUp(const SplineNode& from, const SplineNode& to) const;
  LegDataMap<Spliner3d> BuildFootstepSplineDown(const LegDataMap<Point>& feet_at_switch,
                                                const SplineNode& to) const;



  int GetSplineID(double t_global) const;
  void BuildOneSegment(const SplineNode& from, const SplineNode& to,
                   Spliner3d& pos, Spliner3d& ori,
                   LegDataMap< Spliner3d >& feet_up,
                   LegDataMap< Spliner3d >& feet_down) const;

  friend class HyqSplinerTest_GetSplineID_Test;
};

} // namespace hyq
} // namespace xpp
#endif // _XPP_HYQ_SPLINER_H_
