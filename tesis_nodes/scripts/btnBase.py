#!/usr/bin/env python

import rospy
from kobuki_msgs.msg import SensorState

class Button:
  def __init__(self):
    rospy.init_node("button_activate")
    rospy.loginfo("Se inicia el nodo de activacion de movimiento")
    self.rate = rospy.Rate(10)

    self.core_sub = rospy.Subscriber("/mobile_base/sensors/core", SensorState, self.core_cb)
    self.value = 0

  def core_cb(self, data):
      self.value = data.buttons

  def logger(self):
    if self.value == 1:
      rospy.loginfo("Contar hasta 10")
      for ii in range(10):
        rospy.loginfo(str(ii+1))
      rospy.loginfo("-"*10)

  def core(self):
    while not rospy.is_shutdown():
      self.logger()
      self.rate.sleep()

if __name__ == "__main__":
  btn = Button()
  btn.core()
  rospy.spin()
