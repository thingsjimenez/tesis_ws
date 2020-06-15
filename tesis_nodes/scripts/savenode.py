#!/usr/bin/env python

import rospy
import actionlib
import rosbag
from nav_msgs.msg import Path
from nav_msgs.msg import Odometry
from kobuki_msgs.msg import SensorState
from geometry_msgs.msg import PoseStamped
from move_base_msgs.msg import MoveBaseAction, MoveBaseGoal, MoveBaseActionResult

class GuardarDatos():
	def __init__(self):
		rospy.init_node('save_data_node')
		rospy.loginfo('Se inicia el nodo de guardar datos')

		self.path_pub = rospy.Publisher('/Path_Robot', Path, queue_size=1)
		self.result_sub = rospy.Subscriber("/move_base/result", MoveBaseActionResult, self.result_cb)
		self.odom_sub = rospy.Subscriber('/odom', Odometry, self.odom_cb)
		self.core_sub = rospy.Subscriber('/mobile_base/sensors/core', SensorState, self.core_cb)

		self.path = Path()
		self.core = SensorState()
		self.result = 0
		self.cnt = 0

		self.flag_goal = 0
		self.bag = rosbag.Bag('test{}.bag'.format(self.cnt), 'w')

	def odom_cb(self, data):
		self.path.header = data.header
		pose = PoseStamped()
		pose.header = data.header
		pose.pose = data.pose.pose
		self.path.poses.append(pose)
		self.path_pub.publish(self.path)

		self.bag.write('path_robot', self.path)
		self.bag.write('mobile_base/sensors/core', self.core)

		if (self.result != 0):
			rospy.loginfo(self.result)
			self.result = 0
			self.flag_goal = self.cnt
			self.path.poses = list()
			self.bag.close()
			self.cnt = self.cnt + 1
			self.bag = rosbag.Bag('test{}.bag'.format(self.cnt), 'w')

	def result_cb(self, data):
		self.result = data.status.text

	def core_cb(self, data):
		self.core.header.seq = data.header.seq
		self.core.header.stamp = data.header.stamp
		self.core.header.frame_id = data.header.frame_id
		self.core.time_stamp = data.time_stamp
		self.core.left_encoder = data.left_encoder
		self.core.right_encoder = data.right_encoder
		self.core.left_pwm = data.left_pwm
		self.core.right_pwm = data.right_pwm
		self.core.buttons = data.buttons
		self.core.battery = data.battery
		self.core.current = data.current
		self.core.over_current = data.over_current

if __name__ == '__main__':
	data_bag = GuardarDatos()
	rospy.spin()
