MEMBERS:
Hanna delos Santos
Joseph Dean T. Enriquez
Shanette Giane G. Presas
Jersey Jaclyn K. To


REQUIREMENTS:
C++17 compatible compiler (MSVC, GCC, or Clang)
Windows or Linux terminal (uses console output for interaction)
Visual Studio or VS Code recommended for easier builds

HOW TO RUN:
1. Adjust the config.txt file to your desired configuration (including the number of CPU cores, type of scheduler, duration of quantum cycles, etc.).
2. Debug and run the project on any compatible IDE.
3. Input command "initialize" to initialize the emulator.
4. Input command "scheduler-start" or "scheduler-test" to start the scheduler.
5. Input command "screen -ls" to view a list of all running as well as queued and finished processes.
6. Input command "screen -s <process_name>" to create a new process. 
7. Input command "screen -r <process_name>" to view logs of running process.
8. Input command "process-smi" to print simple information about the process.
9. Input command "report-util" to save the log of a process in a text file.
10. Input command "scheduler-stop" to stop the scheduler.
11. Input command "exit" to exit the program.

How to Open and Build in Visual Studio
1. Open Visual Studio 2022 (or any modern version).
2. Go to File → Open → Project/Solution... if it’s a .sln file.
3. Browse to your project’s root directory and open it.
4. Set the build configuration to Debug or Release.
5. Click Build → Build All.
6. Press Ctrl+F5 (or Debug → Start Without Debugging) to run the emulator.
