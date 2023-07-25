import subprocess
import os

def main():
    

    print("Calling UART2")
    
    while True:
        try:
            subprocess.run("nc -U /embsec/UART2", check=True, shell=True)
            
            break
        except subprocess.CalledProcessError as e:
            print("Loading ...")
    
    
    

if __name__ == "__main__":
    main()