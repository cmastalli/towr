/*
 * zmp_publisher.h
 *
 *  Created on: Apr 5, 2016
 *      Author: winklera
 */

#ifndef USER_TASK_DEPENDS_XPP_OPT_SRC_ZMP_PUBLISHER_H_
#define USER_TASK_DEPENDS_XPP_OPT_SRC_ZMP_PUBLISHER_H_

#include <xpp/ros/i_visualizer.h>

#include <xpp/zmp/continuous_spline_container.h>

#include <xpp/hyq/foothold.h>

#include <ros/ros.h>
#include <visualization_msgs/MarkerArray.h>

#include <Eigen/Dense>
#include <Eigen/StdVector>

namespace xpp {
namespace ros {

class MarkerArrayBuilder : public IVisualizer {

public:
  typedef std::vector<Eigen::Vector2d, Eigen::aligned_allocator<Eigen::Vector2d> > StdVecEigen2d;
  typedef std::vector<xpp::hyq::Foothold> VecFoothold;
  typedef visualization_msgs::Marker Marker;
  typedef visualization_msgs::MarkerArray MarkerArray;
  typedef Eigen::Vector2d Vector2d;
  typedef xpp::zmp::SplineContainer SplineContainer;
  typedef xpp::zmp::SplineContainer::VecSpline VecSpline;

public:
  MarkerArrayBuilder(const std::string& topic = "nlp_zmp_publisher");

  virtual ~MarkerArrayBuilder () {};

  visualization_msgs::MarkerArray BuildMsg(const VecSpline& splines,
                                           const VecFoothold& opt_footholds);

public:
  void AddRvizMessage(
      const VecSpline& splines,
      const VecFoothold& opt_footholds,
      const VecFoothold& start_stance,
      double gap_center_x,
      double gap_width_x,
      double alpha) override;

  void publish() const override { ros_publisher_.publish(zmp_msg_); };

private:
  visualization_msgs::MarkerArray zmp_msg_;
  void AddGoal(MarkerArray& msg,const Vector2d& goal);

  void AddSupportPolygons(visualization_msgs::MarkerArray& msg,
                          const VecFoothold& start_stance,
                          const VecFoothold& footholds) const;
  visualization_msgs::Marker BuildSupportPolygon(const VecFoothold& stance_legs,
                                                 xpp::hyq::LegID leg_id) const;
  void AddStartStance(MarkerArray& msg, const VecFoothold& start_stance);

  void AddCogTrajectory(visualization_msgs::MarkerArray& msg,
                     const VecSpline& splines,
                     const std::vector<xpp::hyq::Foothold>& H_footholds,
                     const std::string& rviz_namespace,
                     double alpha = 1.0);

  void AddZmpTrajectory(visualization_msgs::MarkerArray& msg,
                     const VecSpline& splines,
                     const std::vector<xpp::hyq::Foothold>& H_footholds,
                     const std::string& rviz_namespace,
                     double alpha = 1.0);

  void AddFootholds(
      visualization_msgs::MarkerArray& msg,
      const VecFoothold& H_footholds,
      const std::string& rviz_namespace,
      int32_t type = visualization_msgs::Marker::SPHERE,
      double alpha = 1.0);

  void AddLineStrip(visualization_msgs::MarkerArray& msg,
                    double center_x, double width_x,
                    const std::string& rviz_namespace) const;
  Marker GenerateMarker(Vector2d pos, int32_t type, double size) const;

  std_msgs::ColorRGBA GetLegColor(xpp::hyq::LegID leg) const;

  double walking_height_;
  const std::string frame_id_ = "world";
  ::ros::Publisher ros_publisher_;
};

} /* namespace ros */
} /* namespace xpp */

#endif /* USER_TASK_DEPENDS_XPP_OPT_SRC_ZMP_PUBLISHER_H_ */
