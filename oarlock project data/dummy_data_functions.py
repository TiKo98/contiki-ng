"""
Script to create the dummy data used for proof of concept.
Each function gets the time since the programm startet in full miliseconds
and returns the angle at which the oar sits. 
Angles can be shifted by a constant offset.
"""
import matplotlib.pyplot as plt 

def ideal_stroke(time, offset = 0):
    """
    Ideal stroke with constant acceleration. 20 Strokes/minute
    1 second drive, 2 seconds free-wheeling
    """
    # acceleration function acc(t) = a
    a_drive = 1.8E-4
    a_fw = -4.5E-5
    off_fw = 90

    t = time % 3000 # t := time since start of stroke
    # angle function phi(t) = 0.5*a*t**2
    if t <= 1000:
        return 0.5 * a_drive * (t**2) + offset
    else:
        return 0.5 * a_fw * ((t - 1000)**2) + off_fw + offset

def slow_realistic(time, offset = 0):
    """
    Ideal realistic stroke with plateau. 20 Strokes/minute
    1 second drive, 2 seconds free-wheeling
    """
    # acceleration function acc(t) = a(t-500)**8+k
    a_drive = -4.582541E-26
    k_drive = 1.79E-4

    # free wheeling function f(x) = ax**3+bx**2+cx+d
    a_fw = 2.25E-8
    b_fw = -6.75E-5
    # c_fw = 0
    d_fw = 90

    t = time % 3000 # t := time since start of stroke
    # angle function phi(t) = (a/90)*(t-500)**10 + 0.5*k*t**2
    if t <= 1000:
        return (a_drive / 90) * (t - 500)**10 + 0.5 * k_drive * t ** 2 + offset
    else:
        t = t - 1000
        return a_fw * t**3 + b_fw * t**2 + d_fw + offset

def endzug_betont(time, offset = 0):
    """
    Realistic, nonlinear stroke, most force at the end of the stroke.
    1 second drive, 2 seconds free-wheeling
    """
    # Doesn't work correctly yet. Some error ion the maths.
    # acceleration function acc(t) = a*(t-500)**8+b*t+c
    a_drive = -2.497E-26
    b_drive = 4.361E-8
    c_drive = 9.7539E-5

    # free wheeling function f(x) = ax**3+bx**2+cx+d
    a_fw = 2.25E-8
    b_fw = -6.75E-5
    # c_fw = 0
    d_fw = 90

    t = time % 3000 # t := time since start of stroke
    # angle function phi(t) = (a/90)*(t-500)**10 + b/6 * t**3 + 0.5*c*t**2
    if t <= 1000:
        return (a_drive / 90) * (t - 500)**10 + (b_drive / 6) * t**3 + (c_drive / 2) * t**2 + offset
    else:
        t = t - 1000
        return a_fw * t**3 + b_fw * t**2 + d_fw + offset

def ideal_acceleration(time):

    a_drive = 1.8E-4
    a_fw = -4.5E-5
    t = time % 3000

    return a_drive if t <= 1000 else a_fw

def realistic_acceleration(time):
    a = -4.582541E-26
    k = 1.79E-4
    t = time % 3000
    acc = a*(t-500)**8+k

    return acc if t <= 1000 else -1 * k

def endzug_acceleration(time):
    a = -2.497E-26
    b = 4.361E-8
    c = 9.7539E-5
    t = time % 3000
    acc = a*(t-500)**8+b*t+c

    return acc if t <= 1000 else -1 * c

    
if __name__ == "__main__":
    x_vals = list(range(0, 1020, 20))
    f_vals = [realistic_acceleration(t) for t in x_vals]

    plt.plot(x_vals, f_vals, "bo")
    plt.show()
    # print(endzug_betont(1000))
    