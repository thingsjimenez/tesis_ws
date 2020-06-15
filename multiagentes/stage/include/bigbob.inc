define bigbob position (
	size [ 0.25 0.25 0.3 ]
	block (
		points 6
		point[0] [0.75 0]
		point[1] [1 0.25]
		point[2] [1 0.75]
		point[3] [0.75 1]
		point[4] [0 1]
		point[5] [0 0]
		z [0 0.4]
	)
	drive "diff"
	#-- Sensors
	laser_scan()
	bigbobs_camera()
)

define bigbobs_laserscan sensor (
	size [0.025 0.025 0.025] 
	range [0.5 6]
	fov 30.0
	color_rgba [ 1.0 1.0 0.0 0.5]
	samples 180
)

define laser_scan ranger (
	color "blue"
	bigbobs_laserscan( pose [ 0.5 0.0 -0.2 0.0])
)

define bigbobs_camera camera (
	size [ 0.05 0.05 0.075 ]
	color "green"
	resolution [ 640 360 ]
	range [ 0.3 3.0 ]
	fov [ 32.0 18.0 ]
	pantilt [ 0.0 -3.0 ]
	pose [ 0.0 0.0 0.05 0.0]
)

