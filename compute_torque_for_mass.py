# @desc: this command line app calculates the torque required to lift an object 
#        If not provided, it uses a 5mm motor shaft as a parameter for the torque calculations.
# @args: -w, the mass (kg) of the object to move upwards, required
#        -t, the torque (kg/cm) of the stepper motor, required
#        -r, the radius (mm) of the shaft of the motor, optional
# @usage: python compute_torque_for_weight.py -m 2.2 -t 8.4 
#         python compute_torque_for_weight.py -m 2.2 -t 8.4 -r 2.5

import argparse

# constants
DEFAULT_SHAFT_RADIUS = 0.05 # m
GRAVITATIONAL_ATTRACTION = 9.80665 # m/s^2
REQUIRED_ACCELERATION = 3 # m/s^2

# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-m", "--mass", required=True, type=float, help="mass (kg) of the weight to lift")
ap.add_argument("-t", "--torque", required=True, type=float, help="torque (kg/cm) of your stepper motor")
ap.add_argument("-r", "--radius", type=float, help="(optional) radius of the shaft (cm) of your stepper motor")
args = vars(ap.parse_args())

mass = args["mass"]
stepper_torque = args["torque"]
# shaft_radius = if args["radius"] is not  else 
try:
    shaft_radius = args["radius"] / 100.0 # from cm to m
except TypeError:
    shaft_radius = DEFAULT_SHAFT_RADIUS

print("your object mass is {} kg".format(mass))

# 1. compute force of gravity on object
# F = ma
force = mass * GRAVITATIONAL_ATTRACTION

print('1. you have a {}N force pulling the aluminium bar down'.format(force))
 
# 2. compute torque required to keep the system in a steady state
# Torque = force * radius
steady_torque = force * shaft_radius

print('2. you need a {}Nm torque to keep the system steady'.format(steady_torque))

# 3. to get the real torque you have to specify how fast you want to accelerate
up_force = mass * REQUIRED_ACCELERATION
up_torque = up_force * shaft_radius

print('\t{}N force to move the object up if there was no gravity'.format(up_force))
print('\t{}Nm torque to move the object up if there was no gravity'.format(up_torque))

# 4. this the force we need to fight gravity and push the object upwards
total_force = up_force + force
total_torque = total_force * shaft_radius
# 1 newton meter is equal to 10.197162129779 kg-cm.
total_torque_kgcm = total_torque * 10.197162129779

print('3. you need a:')
print('\t{}N force to move the object up'.format(total_force))
print('\t{}Nm torque to move the object up'.format(total_torque))
print('\t{}Kg/cm torque to move the object up'.format(total_torque_kgcm))

print('\nyour motor has a {}Kg/cm torque, so you would need {} times the torque of your current motor'.format(stepper_torque, total_torque_kgcm/stepper_torque))
