import serial
import time
import numpy as np

# Setup serial connection to Arduino
ser = serial.Serial('/dev/serial0', 9600)  # Adjust the port as necessary

# Q-learning parameters
grid_size = 3
num_states = grid_size * grid_size
num_actions = 4  # forward, backward, left, right
Q_table = np.zeros((num_states, num_actions))
learning_rate = 0.1
discount_factor = 0.9
epsilon = 0.1

# Robot position
current_x, current_y = 0, 0

# Function to get target coordinates from Arduino
def get_target():
    while True:
        if ser.in_waiting > 0:
            data = ser.readline().decode().strip()
            x, y = map(int, data.split(','))
            return x, y

# Receive the goal coordinates from the Arduino
goal_x, goal_y = get_target()

def get_state(x, y):
    return x * grid_size + y

def get_reward(x, y):
    return 10 if x == goal_x and y == goal_y else -1

def choose_action(state):
    if np.random.uniform(0, 1) < epsilon:
        return np.random.randint(num_actions)
    else:
        return np.argmax(Q_table[state])

def move_robot(action):
    if action == 0:
        ser.write(b'F')  # Forward
    elif action == 1:
        ser.write(b'B')  # Backward
    elif action == 2:
        ser.write(b'R')  # Right
    elif action == 3:
        ser.write(b'L')  # Left
    time.sleep(1)  # Adjust delay as necessary
    ser.write(b'S')  # Stop
    time.sleep(1)

def update_position(action):
    global current_x, current_y
    if action == 0:
        current_y += 1
    elif action == 1:
        current_y -= 1
    elif action == 2:
        current_x += 1
    elif action == 3:
        current_x -= 1

def train():
    global current_x, current_y
    for episode in range(1000):
        current_x, current_y = 0, 0
        while (current_x, current_y) != (goal_x, goal_y):
            state = get_state(current_x, current_y)
            action = choose_action(state)
            move_robot(action)
            update_position(action)
            next_state = get_state(current_x, current_y)
            reward = get_reward(current_x, current_y)
            best_next_action_value = np.max(Q_table[next_state])
            Q_table[state, action] += learning_rate * (reward + discount_factor * best_next_action_value - Q_table[state, action])
            if (current_x, current_y) == (goal_x, goal_y):
                print("Reached the goal!")
                ser.write(b'S')  # Stop the robot

if __name__ == "__main__":
    train()
