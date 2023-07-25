import subprocess
import os

def main():
    current_directory = os.path.dirname(os.path.abspath(__file__))

    print("Calling emulator")
    
    subprocess.run(["python", "./bl_emulate.py"], cwd=current_directory)
    while (1):
        s = 1
    
    

if __name__ == "__main__":
    main()