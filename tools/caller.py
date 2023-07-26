import subprocess
import os

if __name__ == "__main__":
    current_directory = os.path.dirname(os.path.abspath(__file__))

    print("Calling emulator")

    subprocess.run(["python", "./bl_emulate.py"], cwd=current_directory)
    while True:
        pass
