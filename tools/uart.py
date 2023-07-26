import subprocess
import sys

if __name__ == "__main__":
    n = sys.argv[1]

    print(f"Calling UART{n}")

    while True:
        try:
            subprocess.run(f"nc -U /embsec/UART{n}", check=True, shell=True)

            break
        except subprocess.CalledProcessError:
            print("Loading ...")
