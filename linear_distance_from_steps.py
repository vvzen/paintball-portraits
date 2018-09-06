# @desc: this command line app calculates the linear motion of a motor that is moved by belts
#        given its steps per revolution and shaft size
#
# @args: -s, the steps needed to achieve one revolution on the motor
#        -r, the radius (mm) of the shaft of the motor
#        -f, the microstepping factor (1, 2, 4, 8, 16 or 32)
#
# @usage: python compute_linear_distance_from_steps.py -s 200 -r 2.5
#
# @credits: thanks to https://www.norwegiancreations.com/2015/07/tutorial-calibrating-stepper-motor-machines-with-belts-and-pulleys/
#           for all the useful infos

import argparse

# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-s", "--steps", required=True, type=int, help="number of steps for one revolution of the motor")
ap.add_argument("-p", "--pitch", required=True, type=float, help="the pitch of the pulley (distance between teeth)")
ap.add_argument("-f", "--factor", required=True, type=int, choices=[1, 2, 4, 8, 16, 32], help="microstepping factor of your stepper driver. Can only be 1,2,4,8,16 or 32")
ap.add_argument("-t", "--teeth", required=True, type=int, help="the number of teeth on your pulley")
args = vars(ap.parse_args())

steps_per_revolution = args["steps"]
pitch = args["pitch"]
microstepping_factor = args["factor"]
number_of_teeth = args["teeth"]

steps_per_mm = (steps_per_revolution * microstepping_factor) / (pitch * number_of_teeth)

print("With your setup you get {} steps per mm".format(steps_per_mm))
