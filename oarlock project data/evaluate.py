"""
Demonstration of how to use real or dummy data to get from the time and angle 
measurements to the acceleration. 
"""
import dummy_data_functions as ddf

import matplotlib.pyplot as plt

# Global variables to store previously measured / calculated values
# [-999, -999] is used for invalid Data
last_angle = [-999, -999]
last_vel = [-999, -999]

def calc_acceleration(measurement):
    """
    Function to calculate the acceleration from the measurements.
    measurement: Array of [Angle, Time]
    """
    # Use globals to have access to previous calculations
    global last_angle
    global last_vel
    # if this is the first measurement, set this as the las_angle and return invalid
    if last_angle == [-999, -999]:
        last_angle = measurement
        return [-999, -999]
    else:
        # if there was a previous angle calculate the velocity using the differential quotient 
        # https://www.mathebibel.de/differentialquotient
        angle_difference = measurement[0] - last_angle[0]
        time_difference = measurement[1] - last_angle[1]
        vel = [angle_difference / time_difference, measurement[1]]
        last_angle = measurement
        
        # if there is no previously calculated velocity set it and return invalid
        if last_vel == [-999, -999]:
            last_vel = vel
            return [-999, -999]
        else:
            # if there was a previous velocity use this to calculate the acceleration
            vel_difference = vel[0] - last_vel[0]
            time_difference = vel[1] - last_vel[1]
            last_vel = vel

            return [vel_difference / time_difference, vel[1]]

###############################################################################################
# Just trying something out here, not really important
###############################################################################################

if __name__ == "__main__":
    for i in range(8):
        # just to test: this should be a constant acceleration
        print(calc_acceleration([i**2, i]))

    # simulate some strokes
    number_of_strokes = 20
    time_per_stroke = 3000
    measurements_per_second = 50
    duration = number_of_strokes * time_per_stroke
    measurements = []
    for t in range(0, duration, int(1000 / measurements_per_second)):
        angle = ddf.ideal_stroke(t)
        measurements.append(list([angle, t]))

    # and calculate the accelerations
    accelerations = [calc_acceleration(m) for m in measurements]
    used_accelerations = []

    # we're only interested in positive accelerations
    for a in accelerations:
        if a[0] >= 0:
            used_accelerations.append(a)


    accs = [a[0] for a in used_accelerations]
    ts = [a[1] for a in used_accelerations]

    plt.plot(ts, accs, "bo")
    plt.show()
