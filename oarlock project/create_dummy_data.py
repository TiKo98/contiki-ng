"""
Script to create the dummy data used for proof of concept.
Each function gets the time since the programm startet in full miliseconds
and returns the angle at which the oar sits. 
Angles can be shifted by a constant offset.
"""

def ideal_stroke(time, offset = 0):
    """
    Ideal stroke with constant acceleration. 20 Strokes/minute
    1 second drive, 2 seconds free-wheeling
    """
    acc_drive = 1.8E-4
    acc_fw = -4.5E-5
    off_fw = 90

    stroketime = time % 3000
    if stroketime <= 1000:
        return 0.5 * acc_drive * (stroketime**2) + offset
    else:
        return 0.5 * acc_fw * ((stroketime - 1000)**2) + off_fw + offset

def slow_realistic(time, offset = 0):
    """
    Ideal realistic stroke with plateau. 20 Strokes/minute
    1 second drive, 2 seconds free-wheeling
    """
    pass