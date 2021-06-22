"""
Creates a file of dummy data. First Column with timestamps, second column with angles
"""
import dummy_data_functions as ddf

def ms_to_time(ms):
    """Creates a Timestamp string from given number of miliseconds"""
    miliseconds = ms % 1000
    seconds = int(ms / 1000) % 60
    minutes = int((ms / 1000) / 60)

    # Timestring format: MM:SS.mmm
    miliseconds_string = "0" * (3 - len(str(miliseconds))) + str(miliseconds)
    seconds_string = "0" * (2 - len(str(seconds))) + str(seconds)
    minutes_string = "0" * (2 - len(str(minutes))) + str(minutes)

    return minutes_string + ":" + seconds_string + "." + miliseconds_string

if __name__ == "__main__":
    number_of_strokes = 20
    time_per_stroke = 3000
    measurements_per_second = 50
    duration = number_of_strokes * time_per_stroke

    # timestamps = [ms_to_time(t) for t in range(duration)]

    # angles = [ddf.ideal_stroke(t) for t in range(duration)]
    f = open("./oarlock project/dummy_data/ideal_only.txt", "wt")
    for i in range(0, duration, int(1000 / measurements_per_second)):
        timestamp = ms_to_time(i)
        angle = ddf.ideal_stroke(i)
        line = timestamp + "\t" + str(angle) + "\n"
        f.write(line)
    f.close()